/* asm/microbeepp.h
 *
 * Microbee Premium Plus specific functions/defines
*/

#ifndef _ASM_MICROBEEPP_H
#define _ASM_MICROBEEPP_H

#define MBPP_ALPHA_PLUS_REG     0x01FC01Cu
#define MBPP_COLOR_PORT_REG     0x01FC008u
#define MBPP_FONT_ROM_REG       0x01FC00Bu
#define MBPP_CRTC_ADDR_REG      0x01FC00Cu
#define MBPP_CRTC_DATA_REG      0x01FC00Du

#define MBPP_SCREEN_RAM_START   0x01FF000u
#define MBPP_ATTR_RAM_START     0x01FF000u
#define MBPP_PCG_RAM_START      0x01FF800u

#define MCF_FLEX_CSAR1_OFFSET   0x8C
#define MCF_FLEX_CSMR1_OFFSET   0x90
#define MCF_FLEX_CSCR1_OFFSET   0x94

/* 300ns Access time */
#define MBPP_CSCR_SLOWIO                0x00d40
/* //0x155D40;  //0x005140; */

/* 775ns (yes, I know... its horrible, but cant do anything about that...)
   Access time */
#define MBPP_CSCR_SLOWACCESS    0x00d40
/*      //0x1FFD40; */

struct mbee_cscr_state {
	u32 			original_cscr1;
	unsigned long    	irq_flags;
};

static inline void mbee_con_set_cscr(struct mbee_cscr_state *s, u32 new_cscr1)
{
        local_irq_save(s->irq_flags);
        local_irq_disable();

	s->original_cscr1 = __raw_readw(MCF_IPSBAR + MCF_FLEX_CSCR1_OFFSET);
        flush_cache_all();
        __raw_writew(new_cscr1, MCF_IPSBAR + MCF_FLEX_CSCR1_OFFSET);
}

static inline void mbee_con_reset_cscr(struct mbee_cscr_state *s)
{
        flush_cache_all();
        __raw_writew(s->original_cscr1, MCF_IPSBAR + MCF_FLEX_CSCR1_OFFSET);
        local_irq_restore(s->irq_flags);
}

#endif /* #ifndef _ASM_MICROBEEPP_H */
