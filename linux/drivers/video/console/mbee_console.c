/* mbee_console.c
 *
 * Microbee Premium Plus Console Driver
 *
 * Copyright (C) 2018 Christopher Collins <kuroneko-linux@sysadninjas.net>
 * Copyright (C) 2012 Microbee Technology
 */

/*
 * The Microbee Console is an interesting mishmash of old and new hardware.
 *
 * It uses a simple character generator using a 6545 CRTC and some SRAM
 * that is local to the Z80 on the mainboard.  There are 8x16 and 6x10 fonts
 * in roms directly coupled to the character generator logic, as well as a
 * few KB of PCG where custom character-sets can be loaded.
 *
 * Access to the character generator is arbitrated through an FPGA attached
 * to the mini-FlexBus and configured via the flexbus1 registers.
 *
 * It's extremely flexable due to it's design - the original Microbees
 * actually drew all their graphics modes by manipulating the PCG memory
 * and writing PCG characters into the screen memory as necessary.
 *
 * Whilst classic VGA can only have one "font" on screen at a time, the
 * microbee can render characters from any PCG bank or the enabled font
 * ROM simultaneously.
 *
 * There's a few issues, however:
 *
 * Whilst hardware windowing/scrollign should be possible with the hardware
 * there's insufficient RAM available to the CRTC to make it happen in 80x24
 * mode.
 *
 * The MiniFlexBus is slow to begin with, and the arbitration with the 
 * FPGA doesn't help things much either.  Console IO is slow and must
 * inhibit interrupts and any kind of preemption until the IOP is 
 * complete.
 *
 * Another headache is that where the VGA-console only has 16 bits of
 * character/attribute data per character cell, the microbee console has
 * 20 bits: 7 bits character, 1 bit PCG enable, 8 bits colour (fg & bg), 
 * 4 bits PCG bank.  We can't actually store the entire console state
 * in the standard off-screen buffer, so PCG is completely ignored right
 * now.
*/

#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/vt_kern.h>
#include <linux/vt_buffer.h>
#include <linux/irqflags.h>
#include <linux/memory.h>
#include <linux/module.h>
#include <asm/io.h>

#include <linux/font.h>

#define MBPP_ALPHA_PLUS_REG 	0x01FC01Cu
#define MBPP_COLOR_PORT_REG	0x01FC008u
#define MBPP_FONT_ROM_REG 	0x01FC00Bu
#define MBPP_CRTC_ADDR_REG	0x01FC00Cu
#define MBPP_CRTC_DATA_REG	0x01FC00Du

#define MBPP_SCREEN_RAM_START	0x01FF000u
#define MBPP_ATTR_RAM_START	0x01FF000u
#define MBPP_PCG_RAM_START	0x01FF800u

#define MCF_FLEX_CSAR1_OFFSET	0x8C
#define MCF_FLEX_CSMR1_OFFSET	0x90
#define MCF_FLEX_CSCR1_OFFSET	0x94

#define MCF_CSCR_WS_MASK	0x0000FC00u
#define MCF_CSCR_WS_SHIFT	10

#define CRTC_CURSOR_START	10
#define CRTC_CURSOR_END		11
#define CRTC_CURSOR_POSITION_H	14
#define CRTC_CURSOR_POSITION_L	15

static u32 original_cscr1;
static unsigned long 	irq_flags;

static inline void mbee_con_set_cscr(u32 new_cscr1)
{
	local_irq_save(irq_flags);
	local_irq_disable();

	__raw_writew(new_cscr1, MCF_IPSBAR + MCF_FLEX_CSCR1_OFFSET);
}

static inline void mbee_con_reset_ws(void)
{
	__raw_writew(original_cscr1, MCF_IPSBAR + MCF_FLEX_CSCR1_OFFSET);
	local_irq_restore(irq_flags);
}

/* 300ns Access time */
#define MBPP_CSCR_SLOWIO		0x00d40
/* //0x155D40;	//0x005140; */

/* 775ns (yes, I know... its horrible, but cant do anything about that...)
   Access time */
#define MBPP_CSCR_SLOWACCESS	0x00d40
/*	//0x1FFD40; */

#define SCREEN_WIDTH		80
#define SCREEN_HEIGHT		24
#define MBPP_CON_SCANLINES	264
#define MBPP_CON_FONTHEIGHT	10
/*
   font notes:
     in 64x16, characters are 8x16
     in 80x24, they are 6x10 with a total of 264 scanlines.
*/

static const char *mbee_console_startup(void)
{
	const char *display_desc = "microbee console";
	// save our start-up CSCR1 configuration so we can set it back at any time.
	original_cscr1 = __raw_readl(MCF_IPSBAR + MCF_FLEX_CSCR1_OFFSET);

	return display_desc;
}

static void mbee_console_init(struct vc_data *c, int init)
{
	c->vc_can_do_color = 1;
	c->vc_cols = SCREEN_WIDTH;
	c->vc_rows = SCREEN_HEIGHT;
	c->vc_scan_lines = MBPP_CON_SCANLINES;
	c->vc_font.height = MBPP_CON_FONTHEIGHT;
	c->vc_complement_mask = 0x7700;
}

static void mbee_console_deinit(struct vc_data *c)
{
}

static void mbee_console_do_clear(int sy, int sx, int height, int width,
		u8 attr_byte, u8 color_byte, u8 screen_byte)
{
	u8	save_ap_reg;
	
	int 		cy;

	mbee_con_set_cscr(MBPP_CSCR_SLOWIO);
	save_ap_reg = __raw_readb(MBPP_ALPHA_PLUS_REG);

	/* reset the attribute (PCG Bank) */
	__raw_writeb(save_ap_reg|0x10, MBPP_ALPHA_PLUS_REG); /* set the Attribute ram access bit.. */
	for (cy = sy; cy < sy+height; cy++) {
		memset_io(MBPP_ATTR_RAM_START + (cy * SCREEN_WIDTH) + sx, attr_byte, width);
	}
	__raw_writeb(save_ap_reg&~0x10, MBPP_ALPHA_PLUS_REG); /* set the Attribute ram access bit.. */

	/* reset the colour */
	__raw_writeb(0x40, MBPP_COLOR_PORT_REG);
	for (cy = sy; cy < sy+height; cy++) {
		memset_io(MBPP_PCG_RAM_START + (cy * SCREEN_WIDTH) + sx, color_byte, width);
	}
	__raw_writeb(0x00, MBPP_COLOR_PORT_REG);

	/* fill the area with blanks. */
	for (cy = sy; cy < sy+height; cy++) {
		memset_io(MBPP_SCREEN_RAM_START + (cy * SCREEN_WIDTH) + sx, screen_byte, width);
	}
	mbee_con_reset_ws();
}

static void mbee_console_clear(struct vc_data *vc, int sy, int sx, int height, int width)
{
	mbee_console_do_clear(sy, sx, height, width, 0, 
		(vc->vc_video_erase_char>>8)&0xFF,
		vc->vc_video_erase_char & 0xFF);
}

static void mbee_console_putc(struct vc_data *vc, int c, int ypos, int xpos)
{
	u8 save_ap_reg;
	u16	offset = ypos * SCREEN_WIDTH + xpos;

	mbee_con_set_cscr(MBPP_CSCR_SLOWIO);
	save_ap_reg = __raw_readb(MBPP_ALPHA_PLUS_REG);

	/* force the ATTR RAM */
	__raw_writeb(save_ap_reg|0x10, MBPP_ALPHA_PLUS_REG);
	__raw_writeb(0, MBPP_ATTR_RAM_START + offset);
	__raw_writeb(save_ap_reg&~0x10, MBPP_ALPHA_PLUS_REG);

	/* write the (colour) attribute */
	__raw_writeb(0x40, MBPP_COLOR_PORT_REG);
	__raw_writeb((c>>8)&0xFF, MBPP_PCG_RAM_START + offset);
	__raw_writeb(0x0, MBPP_COLOR_PORT_REG);

	/* write the character */
	__raw_writeb(c & 0xFF, MBPP_SCREEN_RAM_START + offset);

	mbee_con_reset_ws();
}

static void mbee_console_putcs(struct vc_data *vc, const unsigned short *s,
		int count, int ypos, int xpos)
{
	u8 save_ap_reg;
	u16	offset = ypos * SCREEN_WIDTH + xpos;
	int ctr;
	int c;

	mbee_con_set_cscr(MBPP_CSCR_SLOWIO);
	save_ap_reg = __raw_readb(MBPP_ALPHA_PLUS_REG);

	/* force the ATTR RAM */
	__raw_writeb(save_ap_reg|0x10, MBPP_ALPHA_PLUS_REG);
	memset_io(MBPP_ATTR_RAM_START + offset, 0, count);
	__raw_writeb(save_ap_reg&~0x10, MBPP_ALPHA_PLUS_REG);

	/* write the (colour) attribute + screen RAM concurrently. */
	__raw_writeb(0x40, MBPP_COLOR_PORT_REG);
	for (ctr = 0; ctr < count; ctr++) {
		c = scr_readw(s+ctr);
		__raw_writeb((c>>8)&0xFF, MBPP_PCG_RAM_START+offset+ctr);
		__raw_writeb(c&0xFF, MBPP_SCREEN_RAM_START+offset+ctr);
	}
	__raw_writeb(0x0, MBPP_COLOR_PORT_REG);
	mbee_con_reset_ws();
}

inline static void mbee_console_set_cursor_size(u8 start, u8 end)
{
	__raw_writeb(CRTC_CURSOR_START, MBPP_CRTC_ADDR_REG);
	__raw_writeb(0x40 | (start&0x1F), MBPP_CRTC_DATA_REG);
	__raw_writeb(CRTC_CURSOR_END, MBPP_CRTC_ADDR_REG);
	__raw_writeb(end & 0x1F, MBPP_CRTC_DATA_REG);
}


static void mbee_console_cursor(struct vc_data *c, int mode) 
{
	int cursoffs = ((c->vc_pos - c->vc_visible_origin) / 2);

	mbee_con_set_cscr(MBPP_CSCR_SLOWIO);
	switch (mode) {
	case CM_ERASE:
		/* turn off the cursor */
		__raw_writeb(CRTC_CURSOR_START, MBPP_CRTC_ADDR_REG);
		__raw_writeb(0x20, MBPP_CRTC_DATA_REG);
		break;
	case CM_MOVE:
	case CM_DRAW:
		if (cursoffs >= 0 && cursoffs < 0x800) {
			__raw_writeb(CRTC_CURSOR_POSITION_H, MBPP_CRTC_ADDR_REG);
			__raw_writeb((cursoffs >> 8) & 0x3F, MBPP_CRTC_DATA_REG);
			__raw_writeb(CRTC_CURSOR_POSITION_L, MBPP_CRTC_ADDR_REG);
			__raw_writeb(cursoffs & 0xFF, MBPP_CRTC_DATA_REG);
		}
		switch (c->vc_cursor_type & 0x0f) {
		case CUR_UNDERLINE:
			mbee_console_set_cursor_size(c->vc_font.height-3, c->vc_font.height-2);
			break;
		case CUR_TWO_THIRDS:
			mbee_console_set_cursor_size(c->vc_font.height / 3, c->vc_font.height - 2);
			break;
		case CUR_LOWER_THIRD:
			mbee_console_set_cursor_size(c->vc_font.height * 2 / 3, c->vc_font.height - 2);
			break;
		case CUR_LOWER_HALF:
			mbee_console_set_cursor_size(c->vc_font.height / 2, c->vc_font.height - 2);
			break;
		case CUR_NONE:
			__raw_writeb(CRTC_CURSOR_START, MBPP_CRTC_ADDR_REG);
			__raw_writeb(0x20, MBPP_CRTC_DATA_REG);
			break;
		default:
			mbee_console_set_cursor_size(1, c->vc_font.height - 2);
			break;
		}
	}
	mbee_con_reset_ws();
}

static u8 mbee_console_build_attr(struct vc_data *c, u8 color, u8 intensity,
		u8 blink, u8 underline, u8 reverse, u8 italic)
{
	u8 attr = color;

	if (italic)
		attr = (attr & 0xF0) | c->vc_itcolor;
	else if (underline)
		attr = (attr & 0xf0) | c->vc_ulcolor;
	else if (intensity == 0)
		attr = (attr & 0xf0) | c->vc_halfcolor;	
	if (reverse)
		attr = (attr >> 4) | (attr << 4);

	/* the microbee colour byte is actually flipped on the lower 3 bits
	 * of each nibble compared to VGA's 16 colour palette.
	 *
	 * because everybody kinda expects VGA-esque colours, we'll flip them 
 	 * now.
	 *
	 * (At least the foreground & background are in the same order)
	*/
	attr = (attr & 0x11) << 2 | (attr & 0x44) >> 2 | (attr & ~0x55);
	if (intensity == 2)
		attr ^= 0x08;

	return attr;
}

static int mbee_console_scroll(struct vc_data *vc, int t, int b, int dir,
		int lines)
{
	u8	save_ap_reg;
	int	s_offset = 0, d_offset = 0;
	int	clr_y = 0;
	int	scroll_region_size = (b-t-lines);
	int	bcount = scroll_region_size * SCREEN_WIDTH;

	if (dir == SM_UP) {
		s_offset = (SCREEN_WIDTH * (t + lines));
		d_offset = (SCREEN_WIDTH * t);
		clr_y = b-lines;
	} else {
		s_offset = (SCREEN_WIDTH * t);
		d_offset = (SCREEN_WIDTH * (t + lines));
		clr_y = t;
	}
	/* shunt the data around - character data first */
	mbee_con_set_cscr(MBPP_CSCR_SLOWIO);

	/* attribute memory first */
	save_ap_reg = __raw_readb(MBPP_ALPHA_PLUS_REG);
	__raw_writeb(save_ap_reg|0x10, MBPP_ALPHA_PLUS_REG); //set the Attribute ram access bit..
	memmove((void *)MBPP_PCG_RAM_START + d_offset, (void *)MBPP_PCG_RAM_START + s_offset, bcount);
	__raw_writeb(save_ap_reg&~0x10, MBPP_ALPHA_PLUS_REG); //set the Attribute ram access bit..

	/* reset the colour */
	__raw_writeb(0x40, MBPP_COLOR_PORT_REG);
	memmove((void *)MBPP_PCG_RAM_START + d_offset, (void *)MBPP_PCG_RAM_START + s_offset, bcount);
	__raw_writeb(0x00, MBPP_COLOR_PORT_REG);

	/* shunt the data around */
	memmove((void *)MBPP_SCREEN_RAM_START + d_offset, (void *)MBPP_SCREEN_RAM_START + s_offset, bcount);
	mbee_con_reset_ws();

	/* now, clear the scrolled block */
	mbee_console_clear(vc, clr_y, 0, lines, vc->vc_cols);

	/* return 0 so vt updates it's internal state. */
	return 0;
}

static int mbee_console_switch(struct vc_data *c)
{
	/* no state saving required (since we only have one mode), but
	 * get VT to redraw us. */
	return 1;
}

static int mbee_console_blank(struct vc_data *c, int blank, int mode_switch)
{
	if (blank) {
		mbee_console_do_clear(0, 0, SCREEN_HEIGHT, SCREEN_WIDTH,
			0, 0, ' ');
		return 0;
	}
	return 1;
}

static int mbee_console_scrolldelta(struct vc_data *vc, int lines)
{
        /* there is no off-screen memory, so we can't scroll back */
        return 0;
}


/* Note for myself on what needs to be done still...
	
	 * Prepare the console for the debugger.  This includes, but is not
	 * limited to, unblanking the console, loading an appropriate
	 * palette, and allowing debugger generated output.
	 
	int	(*con_debug_enter)(struct vc_data *);
	
	 * Restore the console to its pre-debug state as closely as possible.
	 
	int	(*con_debug_leave)(struct vc_data *);

*/

const struct consw mbee_con = {
	.owner = 		THIS_MODULE,
	.con_startup =		mbee_console_startup,
	.con_init = 		mbee_console_init,
	.con_deinit = 		mbee_console_deinit,
	.con_clear = 		mbee_console_clear,
	.con_putc =		mbee_console_putc,
	.con_putcs = 		mbee_console_putcs,
	.con_cursor = 		mbee_console_cursor,
	.con_scroll =		mbee_console_scroll,
	.con_switch =		mbee_console_switch,
	.con_blank =		mbee_console_blank,
	.con_scrolldelta =	mbee_console_scrolldelta,
	.con_build_attr =	mbee_console_build_attr,
};
EXPORT_SYMBOL(mbee_con);

MODULE_LICENSE("GPL");
