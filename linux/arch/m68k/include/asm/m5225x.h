/****************************************************************************/

/*
 *	mcf52259.h -- ColdFire 5225x MCU support
 *
 *	(C) Copyright 2003, Greg Ungerer (gerg@snapgear.com)
 * 	(C) Copyright 2018, Christopher Collins
 */

/****************************************************************************/
#ifndef	m5225x_h
#define	m5225x_h
/****************************************************************************/

#define	CPU_NAME		"COLDFIRE(M5225x)"
#define	CPU_INSTR_PER_JIFFY	3
#define	MCF_BUSCLK		(MCF_CLK/2)

/*
 *	Define the 5280/5282 SIM register set addresses.
 */
#define	MCFICM_INTC0		(MCF_IPSBAR + 0x0c00)	/* Base for Interrupt Ctrl 0 */
#define	MCFICM_INTC1		(MCF_IPSBAR + 0x0d00)	/* Base for Interrupt Ctrl 0 */

#define	MCFINTC_IPRH		0x00		/* Interrupt pending 32-63 */
#define	MCFINTC_IPRL		0x04		/* Interrupt pending 1-31 */
#define	MCFINTC_IMRH		0x08		/* Interrupt mask 32-63 */
#define	MCFINTC_IMRL		0x0c		/* Interrupt mask 1-31 */
#define	MCFINTC_INTFRCH		0x10		/* Interrupt force 32-63 */
#define	MCFINTC_INTFRCL		0x14		/* Interrupt force 1-31 */
#define	MCFINTC_IRLR		0x18		/* */
#define	MCFINTC_IACKL		0x19		/* */
#define	MCFINTC_ICR0		0x40		/* Base ICR register */

#define	MCFINT_VECBASE		64		/* Vector base number */
#define	MCFINT_UART0		13		/* Interrupt number for UART0 */
#define	MCFINT_UART1		14		/* Interrupt number for UART1 */
#define	MCFINT_UART2		15		/* Interrupt number for UART2 */
#define MCFINF_I2C0		17		/* Interrupt nubmer for I2C */
#define MCFINF_I2C1		62		/* Interrupt nubmer for I2C */
#define	MCFINT_QSPI		18		/* Interrupt number for QSPI */
#define	MCFINT_FECRX0		23		/* Interrupt number for FEC */
#define	MCFINT_FECTX0		27		/* Interrupt number for FEC */
#define	MCFINT_FECENTC0		29		/* Interrupt number for FEC */
#define	MCFINT_PIT0		55		/* Interrupt number for PIT0 */
#define	MCFINT_PIT1		56		/* Interrupt number for PIT1 */
#define MCFINT_RTC		63		/* Interrupt number for RTC */

#define	MCF_IRQ_UART0	        (MCFINT_VECBASE + MCFINT_UART0)
#define	MCF_IRQ_UART1	        (MCFINT_VECBASE + MCFINT_UART1)
#define	MCF_IRQ_UART2	        (MCFINT_VECBASE + MCFINT_UART2)

#define	MCF_IRQ_FECRX0		(MCFINT_VECBASE + MCFINT_FECRX0)
#define	MCF_IRQ_FECTX0		(MCFINT_VECBASE + MCFINT_FECTX0)
#define	MCF_IRQ_FECENTC0	(MCFINT_VECBASE + MCFINT_FECENTC0)

#define	MCF_IRQ_QSPI		(MCFINT_VECBASE + MCFINT_QSPI)
#define MCF_IRQ_PIT1		(MCFINT_VECBASE + MCFINT_PIT1)

/*
 *	DMA unit base addresses.
 */
#define	MCFDMA_BASE0		(MCF_IPSBAR + 0x00000100)
#define	MCFDMA_BASE1		(MCF_IPSBAR + 0x00000110)
#define	MCFDMA_BASE2		(MCF_IPSBAR + 0x00000120)
#define	MCFDMA_BASE3		(MCF_IPSBAR + 0x00000130)

/*
 *	UART module.
 */
#define	MCFUART_BASE0		(MCF_IPSBAR + 0x00000200)
#define	MCFUART_BASE1		(MCF_IPSBAR + 0x00000240)
#define	MCFUART_BASE2		(MCF_IPSBAR + 0x00000280)

/*
 *	FEC ethernet module.
 */
#define	MCFFEC_BASE0		(MCF_IPSBAR + 0x00001000)
#define	MCFFEC_SIZE0		0x300

/*
 *	QSPI module.
 */
#define	MCFQSPI_BASE		(MCF_IPSBAR + 0x340)
#define	MCFQSPI_SIZE		0x18

/* PQS3 */
#define	MCFQSPI_CS0		((0x0C*8)+3)
// There is no CS1.
//#define	MCFQSPI_CS1		148
/* PQS5 */
#define	MCFQSPI_CS2		((0x0c*8)+5)
/* PQS6 */
/* HACK: there is no CS1, but the driver assumes CS0-CS2 at the least.  We'll treat CS3 as CS1. */
#define	MCFQSPI_CS1		((0x0c*8)+6)

/*
 * 	GPIO registers
 */
#define MCFGPIO_PODR_TE		(MCF_IPSBAR + 0x00100000)
#define MCFGPIO_PODR_TF		(MCF_IPSBAR + 0x00100001)
#define MCFGPIO_PODR_TG		(MCF_IPSBAR + 0x00100002)
#define MCFGPIO_PODR_TH		(MCF_IPSBAR + 0x00100003)
#define MCFGPIO_PODR_TI		(MCF_IPSBAR + 0x00100004)
#define MCFGPIO_PODR_TJ		(MCF_IPSBAR + 0x00100006)
#define MCFGPIO_PODR_NQ		(MCF_IPSBAR + 0x00100008)
#define MCFGPIO_PODR_AN		(MCF_IPSBAR + 0x0010000A)
#define MCFGPIO_PODR_AS		(MCF_IPSBAR + 0x0010000B)
#define MCFGPIO_PODR_QS		(MCF_IPSBAR + 0x0010000C)
#define MCFGPIO_PODR_TA		(MCF_IPSBAR + 0x0010000E)
#define MCFGPIO_PODR_TC		(MCF_IPSBAR + 0x0010000F)
#define MCFGPIO_PODR_UA		(MCF_IPSBAR + 0x00100011)
#define MCFGPIO_PODR_UB		(MCF_IPSBAR + 0x00100012)
#define MCFGPIO_PODR_UC		(MCF_IPSBAR + 0x00100013)
#define MCFGPIO_PODR_DD		(MCF_IPSBAR + 0x00100014)

#define MCFGPIO_PDDR_TE		(MCF_IPSBAR + 0x00100018)
#define MCFGPIO_PDDR_TF		(MCF_IPSBAR + 0x00100019)
#define MCFGPIO_PDDR_TG		(MCF_IPSBAR + 0x0010001A)
#define MCFGPIO_PDDR_TH		(MCF_IPSBAR + 0x0010001B)
#define MCFGPIO_PDDR_TI		(MCF_IPSBAR + 0x0010001C)
#define MCFGPIO_PDDR_TJ		(MCF_IPSBAR + 0x0010001E)
#define MCFGPIO_PDDR_NQ		(MCF_IPSBAR + 0x00100020)
#define MCFGPIO_PDDR_AN		(MCF_IPSBAR + 0x00100022)
#define MCFGPIO_PDDR_AS		(MCF_IPSBAR + 0x00100023)
#define MCFGPIO_PDDR_QS		(MCF_IPSBAR + 0x00100024)
#define MCFGPIO_PDDR_TA		(MCF_IPSBAR + 0x00100026)
#define MCFGPIO_PDDR_TC		(MCF_IPSBAR + 0x00100027)
#define MCFGPIO_PDDR_UA		(MCF_IPSBAR + 0x00100029)
#define MCFGPIO_PDDR_UB		(MCF_IPSBAR + 0x0010002A)
#define MCFGPIO_PDDR_UC		(MCF_IPSBAR + 0x0010002B)
#define MCFGPIO_PDDR_DD		(MCF_IPSBAR + 0x0010002C)

#define MCFGPIO_PPDSDR_TE	(MCF_IPSBAR + 0x00100030)
#define MCFGPIO_PPDSDR_TF	(MCF_IPSBAR + 0x00100031)
#define MCFGPIO_PPDSDR_TG	(MCF_IPSBAR + 0x00100032)
#define MCFGPIO_PPDSDR_TH	(MCF_IPSBAR + 0x00100033)
#define MCFGPIO_PPDSDR_TI	(MCF_IPSBAR + 0x00100034)
#define MCFGPIO_PPDSDR_TJ	(MCF_IPSBAR + 0x00100036)
#define MCFGPIO_PPDSDR_NQ	(MCF_IPSBAR + 0x00100038)
#define MCFGPIO_PPDSDR_AN	(MCF_IPSBAR + 0x0010003A)
#define MCFGPIO_PPDSDR_AS	(MCF_IPSBAR + 0x0010003B)
#define MCFGPIO_PPDSDR_QS	(MCF_IPSBAR + 0x0010003C)
#define MCFGPIO_PPDSDR_TA	(MCF_IPSBAR + 0x0010003E)
#define MCFGPIO_PPDSDR_TC	(MCF_IPSBAR + 0x0010003F)
#define MCFGPIO_PPDSDR_UA	(MCF_IPSBAR + 0x00100041)
#define MCFGPIO_PPDSDR_UB	(MCF_IPSBAR + 0x00100042)
#define MCFGPIO_PPDSDR_UC	(MCF_IPSBAR + 0x00100043)
#define MCFGPIO_PPDSDR_DD	(MCF_IPSBAR + 0x00100044)

#define MCFGPIO_PCLRR_TE	(MCF_IPSBAR + 0x00100048)
#define MCFGPIO_PCLRR_TF	(MCF_IPSBAR + 0x00100049)
#define MCFGPIO_PCLRR_TG	(MCF_IPSBAR + 0x0010004A)
#define MCFGPIO_PCLRR_TH	(MCF_IPSBAR + 0x0010004B)
#define MCFGPIO_PCLRR_TI	(MCF_IPSBAR + 0x0010004C)
#define MCFGPIO_PCLRR_TJ	(MCF_IPSBAR + 0x0010004E)
#define MCFGPIO_PCLRR_NQ	(MCF_IPSBAR + 0x00100050)
#define MCFGPIO_PCLRR_AN	(MCF_IPSBAR + 0x00100052)
#define MCFGPIO_PCLRR_AS	(MCF_IPSBAR + 0x00100053)
#define MCFGPIO_PCLRR_QS	(MCF_IPSBAR + 0x00100054)
#define MCFGPIO_PCLRR_TA	(MCF_IPSBAR + 0x00100056)
#define MCFGPIO_PCLRR_TC	(MCF_IPSBAR + 0x00100057)
#define MCFGPIO_PCLRR_UA	(MCF_IPSBAR + 0x00100059)
#define MCFGPIO_PCLRR_UB	(MCF_IPSBAR + 0x0010005A)
#define MCFGPIO_PCLRR_UC	(MCF_IPSBAR + 0x0010005B)
#define MCFGPIO_PCLRR_DD	(MCF_IPSBAR + 0x0010005C)

#define MCFGPIO_PTEPAR		(MCF_IPSBAR + 0x00100060)
#define MCFGPIO_PTFPAR		(MCF_IPSBAR + 0x00100061)
#define MCFGPIO_PTGPAR		(MCF_IPSBAR + 0x00100062)
#define MCFGPIO_PTIPAR		(MCF_IPSBAR + 0x00100064)
#define MCFGPIO_PNQPAR		(MCF_IPSBAR + 0x00100068)
#define MCFGPIO_PANPAR		(MCF_IPSBAR + 0x0010006A)
#define MCFGPIO_PASPAR		(MCF_IPSBAR + 0x0010006B)
#define MCFGPIO_PQSPAR		(MCF_IPSBAR + 0x0010006C)
#define MCFGPIO_PTAPAR		(MCF_IPSBAR + 0x0010006E)
#define MCFGPIO_PTCPAR		(MCF_IPSBAR + 0x0010006F)
#define MCFGPIO_PUAPAR		(MCF_IPSBAR + 0x00100071)
#define MCFGPIO_PUBPAR		(MCF_IPSBAR + 0x00100072)
#define MCFGPIO_PUCPAR		(MCF_IPSBAR + 0x00100073)
#define MCFGPIO_PDDPAR		(MCF_IPSBAR + 0x00100074)
#define MCFGPIO_CLKOUTPAR	(MCF_IPSBAR + 0x00100077)
#define MCFGPIO_PTHPAR		(MCF_IPSBAR + 0x00100090)


/*
 * PIT timer base addresses.
 */
#define	MCFPIT_BASE0		(MCF_IPSBAR + 0x00150000)
#define	MCFPIT_BASE1		(MCF_IPSBAR + 0x00160000)

/*
 * 	Edge Port registers
 */
#define MCFEPORT_EPPAR		(MCF_IPSBAR + 0x00130000)
#define MCFEPORT_EPDDR		(MCF_IPSBAR + 0x00130002)
#define MCFEPORT_EPIER		(MCF_IPSBAR + 0x00130003)
#define MCFEPORT_EPDR		(MCF_IPSBAR + 0x00130004)
#define MCFEPORT_EPPDR		(MCF_IPSBAR + 0x00130005)
#define MCFEPORT_EPFR		(MCF_IPSBAR + 0x00130006)

/*
 * 	General Purpose Timers registers
 */
#define MCFGPTA_GPTPORT		(MCF_IPSBAR + 0x001A001D)
#define MCFGPTA_GPTDDR		(MCF_IPSBAR + 0x001A001E)
/*
 *
 * definitions for generic gpio support
 *
 */
#define MCFGPIO_PODR		MCFGPIO_PODR_TE	/* port output data */
#define MCFGPIO_PDDR		MCFGPIO_PDDR_TE	/* port data direction */
#define MCFGPIO_PPDR		MCFGPIO_PPDSDR_TE /* port pin data */
#define MCFGPIO_SETR		MCFGPIO_PPDSDR_TE /* set output */
#define MCFGPIO_CLRR		MCFGPIO_PCLRR_TE /* clr output */

#define MCFGPIO_IRQ_MAX		8
#define MCFGPIO_IRQ_VECBASE	MCFINT_VECBASE
#define MCFGPIO_PIN_MAX		168

/*
 *  Reset Control Unit (relative to IPSBAR).
 */
#define	MCF_RCR			(MCF_IPSBAR + 0x110000)
#define	MCF_RSR			(MCF_IPSBAR + 0x110001)

#define	MCF_RCR_SWRESET		0x80		/* Software reset bit */
#define	MCF_RCR_FRCSTOUT	0x40		/* Force external reset */

/****************************************************************************/
#endif	/* mcf52259_h */
