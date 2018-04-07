/* mbee_kbd.c
 *
 * Microbee Premium Plus Keyboard Driver 
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/platform_device.h>
#include <linux/irqflags.h>
#include <asm/io.h>
#include <asm/cacheflush.h>
#include <asm/microbeepp.h>
#include <uapi/linux/input-event-codes.h>

#define MBPP_KBD_POLL_INTERVAL	250
#define MBPP_KBD_ROWS		8
#define MBPP_KBD_COLS		8

#define CRTC_REG_LPEN_H		16
#define CRTC_REG_LPEN_L		17
#define CRTC_REG_UPDATELOC_H	18
#define CRTC_REG_UPDATELOC_L	19
#define CRTC_REG_DUMMY		31

#define CRTC_STATUS_UPDATE_RDY	0x80
#define CRTC_STATUS_LPEN_FULL	0x40
#define CRTC_STATUS_VBLANK	0x20

/*
 * The Microbee keyboard is a 64-key matrix, but unlike most matrix keyboards,
 * the rows are not connected to a parallel input, but both rows and columns
 * are selected using the CRTC memory-access address-lines and they're 
 * connected back to the lightpen input on the CRTC.
 *
 * There's a magic dance required to scan a specific key in the keyboard
 * matrix, made slightly more obnoxious by how the miniflexbus's waitstates 
 * need to be managed as we go.
 *
 * The rough approach:
 *
 * 1. Map the CHARROM to Z80 RAM (Blocking the CRTC automatic address-line 
 * 	scanning)
 * 2. Write the Key's address into bits 9:4 of the CRTC update register.
 * 3. Reset the LPEN_FULL status bit by reading out the LPEN address
 * 4. write to the DUMMY register to initiate a blanking period interogation
 * 5. Wait until the CRTC UPDATE_RDY bit is set.
 * 6. Read the LPEN_FULL bit to see if it was set.
 * 7. Unmap the CHARROM from Z80 RAM
*/

static struct mbee_cscr_state	_cscr_state;

/* the scancode table is derived from the schematics for the Alpha Plus
 * board.
 *
 * Punctuation, however, is mauled a bit because the mbee keyboard layout
 * doesn't match any common production keyboard today.
 *
 * Remappings include:
 *   mbee caret/tilde	 => HID grave/tilde 
 *   mbee colon/asterisk => HID minus/underscore
 *   mbee minus/equals	 => HID equals/plus
 *   mbee semicolon/plus => HID semicolon/colon
 *   mbee @/grave        => HID apostrphe/quote
 *
 * All others match closely enough that they direct map.
 * it is anticipated that if people /really/ want the keys to match
 * the labels, they can be translated using a keymap loaded after boot
 *
 * Shift and Control are also mapped to the LHS only as there is no right
 * key signal separate on the microbee board.
*/
 
static unsigned int	mbee_keycodes[64] = {
	/* col 0 */
	KEY_APOSTROPHE,	KEY_A,		KEY_B,		KEY_C, 
	KEY_D,		KEY_E, 		KEY_F, 		KEY_G,
	/* col 1 */
	KEY_H, 		KEY_I, 		KEY_J, 		KEY_K,
	KEY_L,		KEY_M,		KEY_N,		KEY_O,
	/* col 2 */
	KEY_P,		KEY_Q,		KEY_R,		KEY_S,
	KEY_T,		KEY_U,		KEY_V,		KEY_W,
	/* col 3 */
	KEY_X,		KEY_Y,		KEY_Z,		KEY_LEFTBRACE,
	KEY_BACKSLASH,	KEY_RIGHTBRACE,	KEY_GRAVE,	KEY_DELETE,
	/* col 4 */
	KEY_0,		KEY_1,		KEY_2,		KEY_3,
	KEY_4,		KEY_5,		KEY_6,		KEY_7,
	/* col 5 */
	KEY_8,		KEY_9,		KEY_MINUS,	KEY_SEMICOLON,
	KEY_COMMA,	KEY_EQUAL,	KEY_DOT,	KEY_SLASH,
	/* col 6 */
	KEY_ESC,	KEY_BACKSPACE,	KEY_TAB,	KEY_LINEFEED,
	KEY_ENTER,	KEY_CAPSLOCK,	KEY_BREAK,	KEY_SPACE, 
	/* col 7 */
	KEY_UP, 	KEY_LEFTCTRL, 	KEY_DOWN, 	KEY_LEFT, 
	0 /* 64 */, 	0 /* 63 */, 	KEY_RIGHT, 	KEY_LEFTSHIFT,
};

static inline void mbee_set_update(u16 addr)
{
	__raw_writeb(CRTC_REG_UPDATELOC_H, MBPP_CRTC_ADDR_REG);
	__raw_writeb((addr>>8)&0x3F, MBPP_CRTC_DATA_REG);
	__raw_writeb(CRTC_REG_UPDATELOC_L, MBPP_CRTC_ADDR_REG);
	__raw_writeb(addr & 0xFF, MBPP_CRTC_DATA_REG);
}

struct mbee_kbd_data {
	u8				kbd_state[MBPP_KBD_ROWS];
};

static u8 read_key(u8 keynum)
{
	u16	keyaddr = ((u16)(keynum) << 4);
	u8	x;
	
	mbee_set_update(keyaddr);
	/* read & discard the LPEN status to reset LPEN_FULL */
	__raw_writeb(CRTC_REG_LPEN_L, MBPP_CRTC_ADDR_REG);
	x = __raw_readb(MBPP_CRTC_DATA_REG);
	__raw_writeb(CRTC_REG_DUMMY, MBPP_CRTC_ADDR_REG);
	__raw_writeb(0, MBPP_CRTC_DATA_REG);
	/* spin until the UPDATE_RDY bit is set */
	while (!((x = __raw_readb(MBPP_CRTC_ADDR_REG)) & CRTC_STATUS_UPDATE_RDY))
		;
	return !!(x & CRTC_STATUS_LPEN_FULL);
}

#define KEY_ADDR(c,r)	((c)<<3|(r))

static void mbee_kbd_poll(struct input_polled_dev *dev)
{
	struct mbee_kbd_data *priv = dev->private;

	u8	kbd_state[MBPP_KBD_ROWS] = {0, 0, 0, 0, 0, 0, 0, 0};
	int	c,r;
	u8	keystate;
	u8	keyaddr;
	bool	needs_sync = false;

	mbee_con_set_cscr(&_cscr_state, MBPP_CSCR_SLOWIO);
	/* quick check first, is LPEN_FULL asserted at all?  if not, then
     	 * no keys have been pressed since our last scan.  We can quick-abort,
	 * assert all-keys up, and save ourselves a lot of time spent spinning
	 * on the CRTC.
	 */
	if (!(__raw_readb(MBPP_CRTC_ADDR_REG) & CRTC_STATUS_LPEN_FULL)) {
		goto all_keys_up;
	}

	/* ok - LPEN_FULL is set - something was held down.  Scan the keyboard
	 * matrix
	*/

	__raw_writeb(1, MBPP_FONT_ROM_REG);
	for (c = 0; c < MBPP_KBD_COLS; c++) {
		for (r = 0; r < MBPP_KBD_ROWS; r++) {
			keystate = read_key(KEY_ADDR(c,r));
			if (keystate) {
				kbd_state[r] |= 1<<c;
			}
		}
	}
	__raw_writeb(0, MBPP_FONT_ROM_REG);
	/* and reset the LPEN register so the fast path works next poll */
	__raw_writeb(CRTC_REG_LPEN_L, MBPP_CRTC_ADDR_REG);
	c = __raw_readb(MBPP_CRTC_DATA_REG);
all_keys_up:
	mbee_con_reset_cscr(&_cscr_state);
			
	/* now, go through the keystates and report differences. */
	for (r = 0; r < MBPP_KBD_ROWS; r++) {
		/* look at which bits changed */
		keystate = kbd_state[r] ^ priv->kbd_state[r];
		for (c = 0; c < MBPP_KBD_COLS; c++) {
			if (keystate & (1 << c)) {
				keyaddr = KEY_ADDR(c,r);
				if (!mbee_keycodes[keyaddr]) {
					continue;
				}
				needs_sync = true;
				input_report_key(dev->input,
					mbee_keycodes[keyaddr],
					(kbd_state[r] & (1 << c))?1:0);
			}
		}
		priv->kbd_state[r] = kbd_state[r];
	}
	if (needs_sync)
		input_sync(dev->input);
}

static int __init mbee_kbd_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
        struct input_polled_dev *poll_dev;
	struct mbee_kbd_data *priv;
	int err;

        priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
        if (!priv)
                return -ENOMEM;

        poll_dev = input_allocate_polled_device();
        if (!poll_dev)
                return -ENOMEM;

        poll_dev->private               = priv;
        poll_dev->poll                  = mbee_kbd_poll;
        poll_dev->poll_interval         = MBPP_KBD_POLL_INTERVAL;

        poll_dev->input->name           = "mbee-kbd";
	poll_dev->input->phys		= "mbee-kbd/input0";

        poll_dev->input->id.bustype     = BUS_HOST;
        poll_dev->input->id.vendor      = 0x0001;
        poll_dev->input->id.product     = 0x0001;
        poll_dev->input->id.version     = 0x0100;

        /* input_set_capability(poll_dev->input, EV_MSC, MSC_SCAN); */
        __set_bit(EV_KEY, poll_dev->input->evbit);
	__set_bit(EV_REP, poll_dev->input->evbit);

        platform_set_drvdata(pdev, poll_dev);

        err = input_register_polled_device(poll_dev);
        if (err)
                goto out_err;

	return 0;
out_err:
        input_free_polled_device(poll_dev);
        return err;
}

static const struct of_device_id mbee_kbd_of_match[] = {
        { .compatible = "mbee-kbd", },
        { },
};
MODULE_DEVICE_TABLE(of, mbee_kbd_of_match);

static struct platform_driver mbee_kbd_driver = {
	.probe		= mbee_kbd_probe,
        .driver 	= {
                .name   = "mbee-kbd",
		.of_match_table = mbee_kbd_of_match,
        },
};
module_platform_driver(mbee_kbd_driver);

MODULE_LICENSE("GPL");


