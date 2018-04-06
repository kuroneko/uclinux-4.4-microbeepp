/* mbee_console.c
 *
 * Microbee Premium Plus Console Driver
 *
 * Copyright (C) 2018 Christopher Collins <kuroneko-linux@sysadninjas.net>
 * Copyright (C) 2012 Microbee Technology
 */

#include <linux/console.h>
#include <linux/console_struct.h>
#include <linux/irqflags.h>
#include <asm/io.h>

#define MBPP_ALPHA_PLUS_REG 	0x01FC01C
#define MBPP_COLOR_PORT_REG		0x01FC008
#define MBPP_FONT_ROM_REG 		0x01FC00B
#define MBPP_CRTC_ADDR_REG		0x01FC00C
#define MBPP_CRTC_DATA_REG		0x01FC00D

#define MBPP_SCREEN_RAM_START	0x01FF000
#define MBPP_ATTR_RAM_START		0x01FF000
#define MBPP_PCG_RAM_START		0x01FF800

#define MCF_FLEX_CSAR1_OFFSET	0x8C
#define MCF_FLEX_CSMR1_OFFSET	0x90
#define MCF_FLEX_CSCR1_OFFSET	0x94

static u32 original_cscr1;

#define MCF_CSCR_WS_MASK		0x0000FC00
#define MCF_CSCR_WS_SHIFT		10

#define CRTC_CURSOR_START		10
#define CRTC_CURSOR_END			11
#define CRTC_CURSOR_POSITION_H	14
#define CRTC_CURSOR_POSITION_L	15

static unsigned long 	irq_flags;

static inline void mbee_con_set_cscr(u32 new_cscr1)
{
	local_irq_save(irq_flags);
	local_irq_disable();

	__raw_writew(MCF_IPSBAR + MCF_FLEX_CSCR1_OFFSET, new_cscr1);
}

static inline void mbee_con_reset_ws()
{
	__raw_writew(MCF_IPSBAR + MCF_FLEX_CSCR1_OFFSET, original_cscr1);
	local_irq_restore(irq_flags);
}

//300ns Access time
#define MBPP_CSCR_SLOWIO		0x00d40
	//0x155D40;	//0x005140;

//775ns (yes, I know... its horrible, but cant do anything about that...) Access time
#define MBPP_CSCR_SLOWACCESS	0x00d40
	//0x1FFD40;

#define SCREEN_WIDTH		80
#define SCREEN_HEIGHT		24
#define MBPP_CON_SCANLINES	264
#define MBPP_CON_FONTHEIGHT	10
/*
   font notes:
     in 64x16, characters are 8x16
     in 80x24, they must be 6x10 with a total of 264 scanlines.
*/

static const char *mbee_console_startup(void)
{
	const char *display_desc = "microbee console"
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

static void mbee_console_clear(struct vc_data *vc, int sy, int sx, int height,
		int width)
{
	u8	save_ap_reg;
	
	int 		cy;

	mbee_con_set_cscr(MBPP_CSCR_SLOWIO);
	save_ap_reg = __raw_readb(MBPP_ALPHA_PLUS_REG);

	// reset the attribute (PCG Bank)
	__raw_writeb(MBPP_ALPHA_PLUS_REG, save_ap_reg|0x10); //set the Attribute ram access bit..
	for (cy = sy; cy < sy+height, cy++) {
		memset_io(MBPP_ATTR_RAM_START + (cy * SCREEN_WIDTH) + sx, 0, width);
	}
	__raw_writeb(MBPP_ALPHA_PLUS_REG, save_ap_reg&~0x10); //set the Attribute ram access bit..

	// reset the colour
	__raw_writeb(MBPP_COLOR_PORT_REG, 0x40);
	for (cy = sy; cy < sy+height, cy++) {
		memset_io(MBPP_PCG_RAM_START + (cy * SCREEN_WIDTH) + sx, vc->vc_attr, width);
	}
	__raw_writeb(MBPP_COLOR_PORT_REG, 0x0);

	// fill the area with blanks.
	for (cy = sy; cy < sy+height, cy++) {
		memset_io(MBPP_SCREEN_RAM_START + (cy * SCREEN_WIDTH) + sx, ' ', width);
	}
	mbee_con_reset_ws();
}

static void mbee_console_putc(struct vc_data *vc, int c, int ypos, int xpos)
{
	u8 save_ap_reg;
	u16	offset = ypos * SCREEN_WIDTH + xpos;

	mbee_con_set_cscr(MBPP_CSCR_SLOWIO);
	save_ap_reg = __raw_readb(MBPP_ALPHA_PLUS_REG);

	// force the ATTR RAM
	__raw_writeb(MBPP_ALPHA_PLUS_REG, save_ap_reg|0x10);
	__raw_writeb(MBPP_ATTR_RAM_START + offset, 0);
	__raw_writeb(MBPP_ALPHA_PLUS_REG, save_ap_reg&~0x10);

	// write the attribute
	__raw_writeb(MBPP_COLOR_PORT_REG, 0x40);
	__raw_writeb(MBPP_PCG_RAM_START + offset, (c>>8) & 0xff);
	__raw_writeb(MBPP_COLOR_PORT_REG, 0x0);

	// write the character
	__raw_writeb(MBPP_SCREEN_RAM_START + offset, c&0xff);

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

	// force the ATTR RAM
	__raw_writeb(MBPP_ALPHA_PLUS_REG, save_ap_reg|0x10);
	memset_io(MBPP_ATTR_RAM_START + offset, 0, count);
	__raw_writeb(MBPP_ALPHA_PLUS_REG, save_ap_reg&~0x10);

	// write the attribute + screen RAM concurrently.
	__raw_writeb(MBPP_COLOR_PORT_REG, 0x40);
	for (ctr = 0; ctr < count; ctr++) {
		c = scr_readw(s+ctr);
		__raw_writeb(MBPP_PCG_RAM_START+offset+ctr, (c>>8)&0xFF);
		__raw_writeb(MBPP_SCREEN_RAM_START + offset, c&0xff);
	}
	__raw_writeb(MBPP_COLOR_PORT_REG, 0x0);
	mbee_con_reset_ws();
}

static void mbee_console_cursor(struct vc_data *c, int mode) 
{
	int cursoffs = ((c->vc_pos - c->vc_visible_origin) / 2);
	if (cursoffs >= 0 && cursoffs < 0x800) {
		__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_POSITION_H);
		__raw_writeb(MBPP_CRTC_DATA_REG, (cursoffs >> 8) & 0x3F);
		__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_POSITION_HL);
		__raw_writeb(MBPP_CRTC_DATA_REG, cursoffs & 0xFF);
	}

	mbee_con_set_cscr(MBPP_CSCR_SLOWIO);
	switch (mode) {
	case CM_ERASE:
		/* turn off the cursor */
		// set no cursor in R10.
		__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_START);
		__raw_writeb(MBPP_CRTC_DATA_REG, 0x20);
		break;
	case CM_MOVE:
	case CM_DRAW:
		if (cursoffs >= 0 && cursoffs < 0x800) {
			__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_POSITION_H);
			__raw_writeb(MBPP_CRTC_DATA_REG, (cursoffs >> 8) & 0x3F);
			__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_POSITION_HL);
			__raw_writeb(MBPP_CRTC_DATA_REG, cursoffs & 0xFF);
		}
		switch (c->vc_cursor_type & 0x0f) {
		case CUR_UNDERLINE:
			__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_START);
			__raw_writeb(MBPP_CRTC_DATA_REG, 0x40 | (c->vc_font.height - 3));
			__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_END);
			__raw_writeb(MBPP_CRTC_DATA_REG, c->vc_font.height - 2);
			break;
		case CUR_TWO_THIRDS:
			__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_START);
			__raw_writeb(MBPP_CRTC_DATA_REG, 0x40 | (c->vc_font.height / 3));
			__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_END);
			__raw_writeb(MBPP_CRTC_DATA_REG, c->vc_font.height - 2);
			break;
		case CUR_LOWER_THIRD:
			__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_START);
			__raw_writeb(MBPP_CRTC_DATA_REG, 0x40 | (c->vc_font.height * 2 / 3));
			__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_END);
			__raw_writeb(MBPP_CRTC_DATA_REG, c->vc_font.height - 2);
			break;
		case CUR_LOWER_HALF:
			__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_START);
			__raw_writeb(MBPP_CRTC_DATA_REG, 0x40 | (c->vc_font.height / 2));
			__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_END);
			__raw_writeb(MBPP_CRTC_DATA_REG, c->vc_font.height - 2);
			break;
		case CUR_NONE:
			__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_START);
			__raw_writeb(MBPP_CRTC_DATA_REG, 0x20);
			break;
		default:
			__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_START);
			__raw_writeb(MBPP_CRTC_DATA_REG, 0x40);
			__raw_writeb(MBPP_CRTC_ADDR_REG, CRTC_CURSOR_END);
			__raw_writeb(MBPP_CRTC_DATA_REG, c->vc_font.height-1);
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
	 * because everybody kinda expects VGA-esque colours, we'll flip them now.
	 *
	 * (At least the foreground & background are in the same order)
	*/
	attr = (attr & 0x11) << 2 | (attr & 0x44) >> 2 | (attr & ~0x55);
	if (intensity == 2)
		attr ^= 0x08;

	return attr;
}

/* Note for myself on what needs to be done still...

	int	(*con_scroll)(struct vc_data *, int, int, int, int);
	void	(*con_bmove)(struct vc_data *, int, int, int, int, int, int);
	int	(*con_switch)(struct vc_data *);
	int	(*con_blank)(struct vc_data *, int, int);
	int	(*con_font_set)(struct vc_data *, struct console_font *, unsigned);
	int	(*con_font_get)(struct vc_data *, struct console_font *);
	int	(*con_font_default)(struct vc_data *, struct console_font *, char *);
	int	(*con_font_copy)(struct vc_data *, int);
	int     (*con_resize)(struct vc_data *, unsigned int, unsigned int,
			       unsigned int);
	int	(*con_set_palette)(struct vc_data *, unsigned char *);
	int	(*con_scrolldelta)(struct vc_data *, int);
	int	(*con_set_origin)(struct vc_data *);
	void	(*con_save_screen)(struct vc_data *);
	u8	(*con_build_attr)(struct vc_data *, u8, u8, u8, u8, u8, u8);
	void	(*con_invert_region)(struct vc_data *, u16 *, int);
	u16    *(*con_screen_pos)(struct vc_data *, int);
	unsigned long (*con_getxy)(struct vc_data *, unsigned long, int *, int *);
	
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

	.con_build_attr =	mbee_console_build_attr,
};
EXPORT_SYMBOL(mbee_con);

MODULE_LICENSE("GPL");
