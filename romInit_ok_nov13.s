/* romInit8xx.s - generic PPC 8XX ROM initialization module */


/* defines */

#define	_ASMLANGUAGE
#include "vxWorks.h"
#include "sysLib.h"
#include "asm.h"
#include "config.h"
#include "regs.h"	
#include "drv/multi/ppc860Siu.h"
#include "cacheLib.h"


/* Can't use ROM_ADRS macro with HIADJ and LO macro functions, for PPC */

	/* Exported internal functions */

	.data
	.globl	_romInit	/* start of system code */
	.globl	romInit		/* start of system code */
	.globl	_romInitWarm	/* start of system code */
	.globl	romInitWarm	/* start of system code */

	/* externals */

	.extern romStart	/* system initialization routine */

	.text
	.align 2

/******************************************************************************
*
* romInit - entry point for VxWorks in ROM
*

* romInit
*     (
*     int startType	/@ only used by 2nd entry point @/
*     )

*/

_romInit:
romInit:
	bl	cold		/* jump to the cold boot initialization */
	
_romInitWarm:
romInitWarm:
	bl	start		/* jump to the warm boot initialization */

	/* copyright notice appears at beginning of ROM (in TEXT segment) */

	.ascii   "Copyright 1984-1997 Wind River Systems, Inc."
	.align 2

cold:
	li	r3, BOOT_COLD	/* set cold boot as start type */

	/*
	 * When the PowerPC 860 is powered on, the processor fetches the
	 * instructions located at the address 0x100. We need to jump
	 * from address 0x100 into the actual ROM space.
	 */

	lis	r4, HIADJ(start)		/* load r4 with the address */
	addi	r4, r4, LO(start)		/* of start */

	lis	r5, HIADJ(romInit)		/* load r5 with the address */
	addi	r5, r5, LO(romInit)		/* of romInit() */

	lis	r6, HIADJ(ROM_TEXT_ADRS)	/* load r6 with the address */
	addi	r6, r6, LO(ROM_TEXT_ADRS)	/* of ROM_TEXT_ADRS */

	sub	r4, r4, r5		
	add	r4, r4, r6 

	mtspr	LR, r4				/* save destination address*/
						/* into LR register */
	blr					/* jump to ROM */
			
start:
	/* set the MSR register to a known state */
	
	/*=======MSR  =  0=========*/

	xor	r4, r4, r4		/* clear register R4 */
	mtmsr 	r4			/* cleat the MSR register */

	/* DER - clear the Debug Enable Register======   I did not do it ==== */

        /*	mtspr	DER, r4      */

	/* ICR - clear the Interrupt Cause Register */

	mtspr	ICR, r4

	/* ICTRL - initialize the Intstruction Support Control register */

	lis	r5, HIADJ(0x00000007)
	addi	r5, r5, LO(0x00000007)
	mtspr	ICTRL, r5

	/* disable the instruction/data cache */
	
	lis	r4, HIADJ (CACHE_CMD_DISABLE)		/* load disable cmd */
	addi	r4, r4, LO (CACHE_CMD_DISABLE)
	mtspr	IC_CST, r4				/* disable I cache */
	mtspr	DC_CST, r4				/* disable D cache */

	/* unlock the instruction/data cache */

	lis	r4, HIADJ (CACHE_CMD_UNLOCK_ALL)	/* load unlock cmd */
	addi	r4, r4, LO (CACHE_CMD_UNLOCK_ALL)
	mtspr	IC_CST, r4			/* unlock all I cache lines */
	mtspr	DC_CST, r4			/* unlock all D cache lines */

	/* invalidate the instruction/data cache */

	lis	r4, HIADJ (CACHE_CMD_INVALIDATE)	/* load invalidate cmd*/
	addi	r4, r4, LO (CACHE_CMD_INVALIDATE)
	mtspr	IC_CST, r4		/* invalidate all I cache lines */
	mtspr	DC_CST, r4		/* invalidate all D cache lines */


	/*
	 * initialize the IMMR register before any non-core registers
	 * modification.
	 */

	lis	r4, HIADJ(INTERNAL_MEM_MAP_ADDR)	
	addi	r4, r4, LO(INTERNAL_MEM_MAP_ADDR)
	mtspr	IMMR, r4		/* initialize the IMMR register */

	mfspr	r4, IMMR		/* read it back, to be sure */
	rlwinm  r4, r4, 0, 0, 15	/* only high 16 bits count */

	/* SYPCR - turn off the system protection stuff=====SYPCR=0xffffff88 */


	lis	r5, 	HIADJ( SYPCR_BME   |  SYPCR_SWF |  SYPCR_BMT | SYPCR_BME  )   
	addi 	r5, r5, LO( SYPCR_BME   |  SYPCR_SWF |  SYPCR_BMT | SYPCR_BME  )

/*	li 	r5, (0xffff)
	addi    r5,  r5, (0xff88)     */

	stw	r5, SYPCR(0)(r4)

	/* set the SIUMCR register for important debug port, etc... stuff */


 	lwz	r5, SIUMCR(0)(r4)       /* old value  |0x0003========  */

	lis	r6, 0x0003
	or	r5, r5, r6
	stw	r5, SIUMCR(0)(r4)

/*   ram tabel loading   */
/*
	 * load r6/r7 with the start/end address of the UPM table for a
	 * non-EDO 60ns Dram.
	 */

	lis	r6, HIADJ( upmTable)
	addi	r6, r6, LO(upmTable)

	lis	r7, HIADJ( upmTableEnd)
	addi	r7, r7, LO(upmTableEnd)
	b	upmInit	


upmInit:
	/* init UPMA for memory access */

	sub	r5, r7, r6		/* compute table size */
	srawi	r5, r5, 2		/* in integer size */

	/* convert upmTable to ROM based addressing */

	lis	r7, HIADJ(romInit)	
	addi	r7, r7, LO(romInit)

	lis	r8, HIADJ(ROM_TEXT_ADRS)
	addi	r8, r8, LO(ROM_TEXT_ADRS)

	sub	r6, r6, r7		/* subtract romInit base address */
	add	r6, r6, r8 		/* add in ROM_TEXT_ADRS address */

/* ====================start  UPAM init=================================================== */					
		/*   r9=cmd Command: OP=Write, UPMB, MAD=0 r9=0x008000000----0x0080000   D(63)*/

	lis	r9, HIADJ (MCR_OP_WRITE | MCR_UM_UPMB | MCR_MB_CS4)
	addi	r9, r9, LO(MCR_OP_WRITE | MCR_UM_UPMB | MCR_MB_CS4)

upmWriteLoop:	
	/* write the UPM table in the UPM */

	lwz	r10, 0(r6)		/* get data from table */
	stw	r10, MDR(0)(r4)		/* store the data to MD register */

	stw	r9, MCR(0)(r4)		/* issue command to MCR register */

	addi	r6, r6, 4		/* next entry in the table */
	addi	r9, r9, 1		/* next MAD address */
	cmpw	r9, r5			/* done yet ? */
	blt	upmWriteLoop





	/* TBSCR - initialize the Time Base Status and Control register */

       /*  TBSCR   = ff000200  -===============  |*/
/*	lis 	r5, (0xff00)         
	ori	r5, r5, 0x0200                 */
	
	lis r5,   HIADJ(TBSCR_REFA    |  TBSCR_REFB | TBSCR_TBF )
	
	sth	r5, TBSCR(0)(r4)

	/* set PISCR ==0x0082 ====PISCR_PS | PISCR_PITF========== */

/*	li	r5, 0x0082
	stw	r5, PISCR(0)(r4)	      */

	li  	r5, PISCR_PS     |   PISCR_PITF
	sth	r5, PISCR(0)(r4)
		
	
	/* set the SPLL frequency to 50 Mhz =divide by 16=======*/

	lis 	r5, 0x00000000
	stw	r5, PLPRCR(0)(r4) 


/*  MAR = 0x88-=========================== */
	li	r5, 0x88
	sth	r5, MAR(0)(r4)
	
/*  MCR = do precharge  from 05 and do)  */


	lis 	r5, HIADJ(MCR_OP_RUN |MCR_UM_UPMB | MCR_MB_CS4 |MCR_MCLF_1X |0x5)
	
	addi 	r5, r5, LO(MCR_OP_RUN |MCR_UM_UPMB | MCR_MB_CS4 |MCR_MCLF_1X |0x5)
	stw 	r5, MCR(0)(r4)

/*  MCR =  do refresh from 30)  */



	lis 	r5, HIADJ(MCR_OP_RUN |MCR_UM_UPMB | MCR_MB_CS4 |MCR_MCLF_1X |0x30)
	
	addi 	r5, r5, LO(MCR_OP_RUN |MCR_UM_UPMB | MCR_MB_CS4 |MCR_MCLF_1X |0x30)
	stw 	r5, MCR(0)(r4)


	/* ==================only one bank=====MPTPR = ox0400== */
	li	r5, 0x0400
	li	r5, MPTPR_PTP_DIV16
	sth	r5,  MPTPR(0)(r4)  
	

	/* Machine B mode register setup====0xd0802114 */

                 	
	lis	r5, HIADJ(0xd0802114)
	addi	r5,   r5, LO(0xd0802114)
	stw	r5, MBMR(0)(r4)


	/* lis 	r5, HIADJ ( (0x13  <<  */ 
/*	stw	r5, MBMR(0)(r4)  */


/*	lis	r5, HIADJ( (0x13 << MAMR_PTA_SHIFT) | MAMR_PTAE |	       \
			  MAMR_AMA_TYPE_2 | MAMR_DSA_1_CYCL | MAMR_G0CLA_A12 | \
			  MAMR_GPL_A4DIS  | MAMR_RLFA_1X    | MAMR_WLFA_1X   | \
			  MAMR_TLFA_4X)
	addi	r5, r5, LO((0x13 << lMAMR_PTA_SHIFT) | MAMR_PTAE |	       \
			  MAMR_AMA_TYPE_2 | MAMR_DSA_1_CYCL | MAMR_G0CLA_A12 | \
			  MAMR_GPL_A4DIS  | MAMR_RLFA_1X    | MAMR_WLFA_1X   | \
			  MAMR_TLFA_4X)
	stw	r5, MAMR(0)(r4)

*/




	/* Map bank 0 to the flash area======br0=0x010000001 ==  */
	
	lis	r5,  0x0100 
	addi	r5, r5, 0x0001
	stw	r5,  BR0(0)(r4)
		

	/* TODO - setup the bank 0 configuration or0=oxffc0 0962======= *	/
	lis 	r5, HIADJ(0xffc0)
	addi	r5, LO(0x0962)
	stw	r5,  OR0(0)(r4)

	
	/*setup the bank 4 configuration br4=0x000000c1  or4=0xffc0 0a00 =====*/
	
	li 	r5, 0x00c1
	stw	r5,  BR4(0)(r4)
	
	lis	r5, HIADJ(0xffc00acc)
	addi 	r5, r5, LO(0xffc00a00)
	stw 	r5,  OR4(0)(r4)
	

	
/* add in ram test========================================== */

	li r5,0x2000
	lis r6,HIADJ(0xaa55aa55)
	addi r6, r6, LO(0xaa55aa55)
	stw r6, 0(r5)
	stw r6, 4(r5)
	stw r6, 8(r5)

/*  ======= ram test  finish==============================   */
        /* initialize the stack pointer */

	lis	sp, HIADJ(STACK_ADRS)
	addi	sp, sp, LO(STACK_ADRS)
	
        /* initialize r2 and r13 according to EABI standard */

	/* go to C entry point */

	addi	sp, sp, -FRAMEBASESZ		/* get frame stack */

	/* 
	 * calculate C entry point: routine - entry point + ROM base 
	 * routine	= romStart
	 * entry point	= romInit	= R7
	 * ROM base	= ROM_TEXT_ADRS = R8
	 * C entry point: romStart - R7 + R8 
	 */

        lis	r6, HIADJ(romStart)	
        addi	r6, r6, LO(romStart)		/* load R6 with C entry point */

	sub	r6, r6, r7			/* routine - entry point */
	add	r6, r6, r8 			/* + ROM base */

	mtlr	r6				/* move C entry point to LR */
	blr					/* jump to the C entry point */






	/* UPM initialization table, 60ns, 50MHz */
upmTable:

	/*
	 * Single Read.		 (Offset 0 in UPMB RAM)
	 */
	.long	0x1f07fc04,	0xeeaefc04,	0x11adfc04,	0xefbbbc00,
	.long 0x1ff77c47,	0x1ff77c34,	0xefeabc34,	0x1fb57c35,
	/*
	 * Burst Read.			 (Offset 8 in UPMB RAM)
	 */
	.long 0x1f07fc04,	0xeeaefc04,	0x10adfc04,	0xf0affc00,
	.long 0xf0affc00,	0xf1affc00,	0xefbbbc00,	0x1ff77c47, 
	.long 0xcff0ff80,	0x3ff0ff00,	0xffffff00,	0xffffffb0,
	.long 0xcfc03f30,	0x3f303f00,	0xccf3ff00,	0x30f7ff07,
	/*
	 * Single Write. 		(Offset 18 in UPMB RAM)
	 */
	.long 0x1f27fc04,	0xeeaebc00,	0x01b93c04,	0x1ff77c47,
	.long 0xffffff07,	0x00000000,	0x00000000,	0x00000000,
	/*
	 * Burst Write. 		(Offset 20 in UPMB RAM)
	 */
	.long 0x1f07fc04,	0xeeaebc00,	0x10ad7c00,	0xf0affc00,
	.long 0xf0affc00,	0xe1bbbc04,	0x1ff77c47,	0x00000000,
	.long 0x00000000,	0x00000000,	0x00000000,	0x00000000,
	.long 0x00000000,	0x00000000,	0x00000000,	0x00000000,
	/*
	 * Refresh  			(Offset 30 in UPMB RAM)
	 */
	.long 0x1ff5fc84,	0xfffffc04,	0xfffffc04,	0xfffffc84,
	.long 0xfffffc07,	0x00000000,	0x00000000,	0x00000000,
	.long 0x00000000,	0x00000000,	0x00000000,	0x00000000,
	/*
	 * Exception. 			(Offset 3c in UPMB RAM)
	 */
	.long 0x7ffffc07,	0x00000000,	0x00000000,	0x00000000,
upmTableEnd:



