/*
 * (C) Copyright 2004-2009
 * Texas Instruments, <www.ti.com>
 * Richard Woodruff <r-woodruff2@ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <asm/arch/cpu.h>
#include <asm/io.h>
#include <asm/arch/bits.h>
#include <asm/arch/mux.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>
#include <asm/arch/clocks.h>
#include <asm/arch/mem.h>
#include <i2c.h>
#include <asm/mach-types.h>
#if (CONFIG_COMMANDS & CFG_CMD_NAND) && defined(CFG_NAND_LEGACY)
#include <linux/mtd/nand_legacy.h>
#endif

#define CONFIG_OMAP4_SDC		1

/* EMIF and DMM registers */
#define EMIF1_BASE			0x4c000000
#define EMIF2_BASE			0x4d000000
#define DMM_BASE			0x4e000000
/* EMIF */
#define EMIF_MOD_ID_REV			0x0000
#define EMIF_STATUS			0x0004
#define EMIF_SDRAM_CONFIG		0x0008
#define EMIF_LPDDR2_NVM_CONFIG		0x000C
#define EMIF_SDRAM_REF_CTRL		0x0010
#define EMIF_SDRAM_REF_CTRL_SHDW	0x0014
#define EMIF_SDRAM_TIM_1		0x0018
#define EMIF_SDRAM_TIM_1_SHDW		0x001C
#define EMIF_SDRAM_TIM_2		0x0020
#define EMIF_SDRAM_TIM_2_SHDW		0x0024
#define EMIF_SDRAM_TIM_3		0x0028
#define EMIF_SDRAM_TIM_3_SHDW		0x002C
#define EMIF_LPDDR2_NVM_TIM		0x0030
#define EMIF_LPDDR2_NVM_TIM_SHDW	0x0034
#define EMIF_PWR_MGMT_CTRL		0x0038
#define EMIF_PWR_MGMT_CTRL_SHDW		0x003C
#define EMIF_LPDDR2_MODE_REG_DATA	0x0040
#define EMIF_LPDDR2_MODE_REG_CFG	0x0050
#define EMIF_L3_CONFIG			0x0054
#define EMIF_L3_CFG_VAL_1		0x0058
#define EMIF_L3_CFG_VAL_2		0x005C
#define IODFT_TLGC			0x0060
#define EMIF_PERF_CNT_1			0x0080
#define EMIF_PERF_CNT_2			0x0084
#define EMIF_PERF_CNT_CFG		0x0088
#define EMIF_PERF_CNT_SEL		0x008C
#define EMIF_PERF_CNT_TIM		0x0090
#define EMIF_READ_IDLE_CTRL		0x0098
#define EMIF_READ_IDLE_CTRL_SHDW	0x009c
#define EMIF_ZQ_CONFIG			0x00C8
#define EMIF_DDR_PHY_CTRL_1		0x00E4
#define EMIF_DDR_PHY_CTRL_1_SHDW	0x00E8
#define EMIF_DDR_PHY_CTRL_2		0x00EC

#define DMM_LISA_MAP_0 			0x0040
#define DMM_LISA_MAP_1 			0x0044
#define DMM_LISA_MAP_2 			0x0048
#define DMM_LISA_MAP_3 			0x004C

#define MR0_ADDR			0
#define MR1_ADDR			1
#define MR2_ADDR			2
#define MR4_ADDR			4
#define MR10_ADDR			10
#define MR16_ADDR			16
#define REF_EN				0x40000000
/* defines for MR1 */
#define MR1_BL4				2
#define MR1_BL8				3
#define MR1_BL16			4

#define MR1_BT_SEQ			0
#define BT_INT				1

#define MR1_WC				0
#define MR1_NWC				1

#define MR1_NWR3			1
#define MR1_NWR4			2
#define MR1_NWR5			3
#define MR1_NWR6			4
#define MR1_NWR7			5
#define MR1_NWR8			6

#define MR1_VALUE	(MR1_NWR3 << 5) | (MR1_WC << 4) | (MR1_BT_SEQ << 3)  \
							| (MR1_BL8 << 0)

/* defines for MR2 */
#define MR2_RL3_WL1			1
#define MR2_RL4_WL2			2
#define MR2_RL5_WL2			3
#define MR2_RL6_WL3			4

/* defines for MR10 */
#define MR10_ZQINIT			0xFF
#define MR10_ZQRESET			0xC3
#define MR10_ZQCL			0xAB
#define MR10_ZQCS			0x56


/* TODO: FREQ update method is not working so shadow registers programming
 * is just for same of completeness. This would be safer if auto
 * trasnitions are working
 */
#define FREQ_UPDATE_EMIF
/* EMIF Needs to be configured@19.2 MHz and shadow registers
 * should be programmed for new OPP.
 */
/* Elpida 2x2Gbit */
#define SDRAM_CONFIG_INIT		0x80800EB1
#define DDR_PHY_CTRL_1_INIT		0x849FFFF5
#define READ_IDLE_CTRL			0x000501FF
#define PWR_MGMT_CTRL			0x4000000f
#define PWR_MGMT_CTRL_OPP100		0x4000000f
#define ZQ_CONFIG			0x500b3215

#define CS1_MR(mr)	((mr) | 0x80000000)
struct ddr_regs{
	u32 tim1;
	u32 tim2;
	u32 tim3;
	u32 phy_ctrl_1;
	u32 ref_ctrl;
	u32 config_init;
	u32 config_final;
	u32 zq_config;
	u8 mr1;
	u8 mr2;
};
const struct ddr_regs ddr_regs_380_mhz = {
	.tim1		= 0x10cb061a,
	.tim2		= 0x20350d52,
	.tim3		= 0x00b1431f,
	.phy_ctrl_1	= 0x849FF408,
	.ref_ctrl	= 0x000005ca,
	.config_init	= 0x80000eb1,
	.config_final	= 0x80001ab1,
	.zq_config	= 0x500b3215,
	.mr1		= 0x83,
	.mr2		= 0x4
};

/*
 * Unused timings - but we may need them later
 * Keep them commented
 */
#if 0
const struct ddr_regs ddr_regs_400_mhz = {
	.tim1		= 0x10eb065a,
	.tim2		= 0x20370dd2,
	.tim3		= 0x00b1c33f,
	.phy_ctrl_1	= 0x849FF408,
	.ref_ctrl	= 0x00000618,
	.config_init	= 0x80000eb1,
	.config_final	= 0x80001ab1,
	.zq_config	= 0x500b3215,
	.mr1		= 0x83,
	.mr2		= 0x4
};

const struct ddr_regs ddr_regs_200_mhz = {
	.tim1		= 0x08648309,
	.tim2		= 0x101b06ca,
	.tim3		= 0x0048a19f,
	.phy_ctrl_1	= 0x849FF405,
	.ref_ctrl	= 0x0000030c,
	.config_init	= 0x80000eb1,
	.config_final	= 0x80000eb1,
	.zq_config	= 0x500b3215,
	.mr1		= 0x23,
	.mr2		= 0x1
};
#endif

const struct ddr_regs ddr_regs_200_mhz_2cs = {
	.tim1		= 0x08648309,
	.tim2		= 0x101b06ca,
	.tim3		= 0x0048a19f,
	.phy_ctrl_1	= 0x849FF405,
	.ref_ctrl	= 0x0000030c,
	.config_init	= 0x80000eb9,
	.config_final	= 0x80000eb9,
	.zq_config	= 0xD00b3215,
	.mr1		= 0x23,
	.mr2		= 0x1
};

/*******************************************************
 * Routine: delay
 * Description: spinning delay to use before udelay works
 ******************************************************/
static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n"
			  "bne 1b" : "=r" (loops) : "0"(loops));
}


void big_delay(unsigned int count)
{
	int i;
	for (i=0; i<count; i++)
		delay(1);
}

/* TODO: FREQ update method is not working so shadow registers programming
 * is just for same of completeness. This would be safer if auto
 * trasnitions are working
 */
static int emif_config(unsigned int base)
{
	unsigned int reg_value, rev;
	const struct ddr_regs *ddr_regs;
	rev = omap_revision();

	if(rev == OMAP4430_ES1_0)
		ddr_regs = &ddr_regs_380_mhz;
	else if (rev == OMAP4430_ES2_0)
		ddr_regs = &ddr_regs_200_mhz_2cs;
	/*
	 * set SDRAM CONFIG register
	 * EMIF_SDRAM_CONFIG[31:29] REG_SDRAM_TYPE = 4 for LPDDR2-S4
	 * EMIF_SDRAM_CONFIG[28:27] REG_IBANK_POS = 0
	 * EMIF_SDRAM_CONFIG[13:10] REG_CL = 3
	 * EMIF_SDRAM_CONFIG[6:4] REG_IBANK = 3 - 8 banks
	 * EMIF_SDRAM_CONFIG[3] REG_EBANK = 0 - CS0
 	 * EMIF_SDRAM_CONFIG[2:0] REG_PAGESIZE = 2  - 512- 9 column
	 * JDEC specs - S4-2Gb --8 banks -- R0-R13, C0-c8
	 */
	*(volatile int*)(base + EMIF_LPDDR2_NVM_CONFIG) &= 0xBFFFFFFF;
	*(volatile int*)(base + EMIF_SDRAM_CONFIG) = ddr_regs->config_init;

	/* PHY control values */
	*(volatile int*)(base + EMIF_DDR_PHY_CTRL_1) = DDR_PHY_CTRL_1_INIT;
	*(volatile int*)(base + EMIF_DDR_PHY_CTRL_1_SHDW)= ddr_regs->phy_ctrl_1;

	/*
	 * EMIF_READ_IDLE_CTRL
	 */
	*(volatile int*)(base + EMIF_READ_IDLE_CTRL) = READ_IDLE_CTRL;
	*(volatile int*)(base + EMIF_READ_IDLE_CTRL_SHDW) = READ_IDLE_CTRL;

	/*
	 * EMIF_SDRAM_TIM_1
	 */
	*(volatile int*)(base + EMIF_SDRAM_TIM_1) = ddr_regs->tim1;
	*(volatile int*)(base + EMIF_SDRAM_TIM_1_SHDW) = ddr_regs->tim1;

	/*
	 * EMIF_SDRAM_TIM_2
	 */
	*(volatile int*)(base + EMIF_SDRAM_TIM_2) = ddr_regs->tim2;
	*(volatile int*)(base + EMIF_SDRAM_TIM_2_SHDW) = ddr_regs->tim2;

	/*
	 * EMIF_SDRAM_TIM_3
	 */
	*(volatile int*)(base + EMIF_SDRAM_TIM_3) = ddr_regs->tim3;
	*(volatile int*)(base + EMIF_SDRAM_TIM_3_SHDW) = ddr_regs->tim3;

	*(volatile int*)(base + EMIF_ZQ_CONFIG) = ddr_regs->zq_config;
	/*
	 * EMIF_PWR_MGMT_CTRL
	 */
	//*(volatile int*)(base + EMIF_PWR_MGMT_CTRL) = PWR_MGMT_CTRL;
	//*(volatile int*)(base + EMIF_PWR_MGMT_CTRL_SHDW) = PWR_MGMT_CTRL_OPP100;
	/*
	 * poll MR0 register (DAI bit)
	 * REG_CS[31] = 0 -- Mode register command to CS0
	 * REG_REFRESH_EN[30] = 1 -- Refresh enable after MRW
	 * REG_ADDRESS[7:0] = 00 -- Refresh enable after MRW
	 */

	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_CFG) = MR0_ADDR;
	do {
		reg_value = *(volatile int*)(base + EMIF_LPDDR2_MODE_REG_DATA);
	} while ((reg_value & 0x1) != 0);

	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_CFG) = CS1_MR(MR0_ADDR);
	do {
		reg_value = *(volatile int*)(base + EMIF_LPDDR2_MODE_REG_DATA);
	} while ((reg_value & 0x1) != 0);


	/* set MR10 register */
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_CFG)= MR10_ADDR;
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_DATA) = MR10_ZQINIT;
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_CFG) = CS1_MR(MR10_ADDR);
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_DATA) = MR10_ZQINIT;

	/* wait for tZQINIT=1us  */
	delay(10);

	/* set MR1 register */
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_CFG)= MR1_ADDR;
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_DATA) = ddr_regs->mr1;
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_CFG) = CS1_MR(MR1_ADDR);
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_DATA) = ddr_regs->mr1;


	/* set MR2 register RL=6 for OPP100 */
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_CFG)= MR2_ADDR;
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_DATA) = ddr_regs->mr2;
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_CFG) = CS1_MR(MR2_ADDR);
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_DATA) = ddr_regs->mr2;

	/* Set SDRAM CONFIG register again here with final RL-WL value */
	*(volatile int*)(base + EMIF_SDRAM_CONFIG) = ddr_regs->config_final;
	*(volatile int*)(base + EMIF_DDR_PHY_CTRL_1) = ddr_regs->phy_ctrl_1;

	/*
	 * EMIF_SDRAM_REF_CTRL
	 * refresh rate = DDR_CLK / reg_refresh_rate
	 * 3.9 uS = (400MHz)	/ reg_refresh_rate
	 */
	*(volatile int*)(base + EMIF_SDRAM_REF_CTRL) = ddr_regs->ref_ctrl;
	*(volatile int*)(base + EMIF_SDRAM_REF_CTRL_SHDW) = ddr_regs->ref_ctrl;

	/* set MR16 register */
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_CFG)= MR16_ADDR | REF_EN;
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_DATA) = 0;
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_CFG) =
						 CS1_MR(MR16_ADDR | REF_EN);
	*(volatile int*)(base + EMIF_LPDDR2_MODE_REG_DATA) = 0;
	/* LPDDR2 init complete */

}
/*****************************************
 * Routine: ddr_init
 * Description: Configure DDR
 * EMIF1 -- CS0 -- DDR1 (256 MB)
 * EMIF2 -- CS0 -- DDR2 (256 MB)
 *****************************************/
static void ddr_init(void)
{
	unsigned int base_addr, rev;
	rev = omap_revision();

	if (rev == OMAP4430_ES1_0)
	{
		/* Configurte the Control Module DDRIO device */
		__raw_writel(0x1c1c1c1c, 0x4A100638);
		__raw_writel(0x1c1c1c1c, 0x4A10063c);
		__raw_writel(0x1c1c1c1c, 0x4A100640);
		__raw_writel(0x1c1c1c1c, 0x4A100648);
		__raw_writel(0x1c1c1c1c, 0x4A10064c);
		__raw_writel(0x1c1c1c1c, 0x4A100650);
	} else if (rev == OMAP4430_ES2_0) {
		__raw_writel(0x9e9e9e9e, 0x4A100638);
		__raw_writel(0x9e9e9e9e, 0x4A10063c);
		__raw_writel(0x9e9e9e9e, 0x4A100640);
		__raw_writel(0x9e9e9e9e, 0x4A100648);
		__raw_writel(0x9e9e9e9e, 0x4A10064c);
		__raw_writel(0x9e9e9e9e, 0x4A100650);
	}
	/* LPDDR2IO set to NMOS PTV */
	__raw_writel(0x00ffc000, 0x4A100704);


	/*
	 * DMM Configuration
	 */

	/* Both EMIFs 128 byte interleaved*/
	if (rev == OMAP4430_ES1_0)
		*(volatile int*)(DMM_BASE + DMM_LISA_MAP_0) = 0x80540300;
	else if (rev == OMAP4430_ES2_0)
		*(volatile int*)(DMM_BASE + DMM_LISA_MAP_0) = 0x80640300;

	/* EMIF2 only at 0x90000000 */
	//*(volatile int*)(DMM_BASE + DMM_LISA_MAP_1) = 0x90400200;

	*(volatile int*)(DMM_BASE + DMM_LISA_MAP_2) = 0x00000000;
	*(volatile int*)(DMM_BASE + DMM_LISA_MAP_3) = 0xFF020100;

	/* DDR needs to be initialised @ 19.2 MHz
	 * So put core DPLL in bypass mode
	 * Configure the Core DPLL but don't lock it
	 */
	configure_core_dpll_no_lock();

	/* No IDLE: BUG in SDC */
	//sr32(CM_MEMIF_CLKSTCTRL, 0, 32, 0x2);
	//while(((*(volatile int*)CM_MEMIF_CLKSTCTRL) & 0x700) != 0x700);
	*(volatile int*)(EMIF1_BASE + EMIF_PWR_MGMT_CTRL) = 0x0;
	*(volatile int*)(EMIF2_BASE + EMIF_PWR_MGMT_CTRL) = 0x0;

	base_addr = EMIF1_BASE;
	emif_config(base_addr);

	/* Configure EMIF24D */
	base_addr = EMIF2_BASE;
	emif_config(base_addr);
	/* Lock Core using shadow CM_SHADOW_FREQ_CONFIG1 */
	lock_core_dpll_shadow();
	/* TODO: SDC needs few hacks to get DDR freq update working */

	/* Set DLL_OVERRIDE = 0 */
	*(volatile int*)CM_DLL_CTRL = 0x0;

	delay(200);

	/* Check for DDR PHY ready for EMIF1 & EMIF2 */
	while((((*(volatile int*)(EMIF1_BASE + EMIF_STATUS))&(0x04)) != 0x04) \
	|| (((*(volatile int*)(EMIF2_BASE + EMIF_STATUS))&(0x04)) != 0x04));

	/* Reprogram the DDR PYHY Control register */
	/* PHY control values */

	sr32(CM_MEMIF_EMIF_1_CLKCTRL, 0, 32, 0x1);
        sr32(CM_MEMIF_EMIF_2_CLKCTRL, 0, 32, 0x1);

	/* Put the Core Subsystem PD to ON State */

	/* No IDLE: BUG in SDC */
	//sr32(CM_MEMIF_CLKSTCTRL, 0, 32, 0x2);
	//while(((*(volatile int*)CM_MEMIF_CLKSTCTRL) & 0x700) != 0x700);
	*(volatile int*)(EMIF1_BASE + EMIF_PWR_MGMT_CTRL) = 0x80000000;
	*(volatile int*)(EMIF2_BASE + EMIF_PWR_MGMT_CTRL) = 0x80000000;

	/* SYSTEM BUG:
	 * In n a specific situation, the OCP interface between the DMM and
	 * EMIF may hang.
	 * 1. A TILER port is used to perform 2D burst writes of
	 * 	 width 1 and height 8
	 * 2. ELLAn port is used to perform reads
	 * 3. All accesses are routed to the same EMIF controller
	 *
	 * Work around to avoid this issue REG_SYS_THRESH_MAX value should
	 * be kept higher than default 0x7. As per recommondation 0x0A will
	 * be used for better performance with REG_LL_THRESH_MAX = 0x00
	 */
	*(volatile int*)(EMIF1_BASE + EMIF_L3_CONFIG) = 0x0A0000FF;
	*(volatile int*)(EMIF2_BASE + EMIF_L3_CONFIG) = 0x0A0000FF;

	/*
	 * DMM : DMM_LISA_MAP_0(Section_0)
	 * [31:24] SYS_ADDR 		0x80
	 * [22:20] SYS_SIZE		0x7 - 2Gb
	 * [19:18] SDRC_INTLDMM		0x1 - 128 byte
	 * [17:16] SDRC_ADDRSPC 	0x0
	 * [9:8] SDRC_MAP 		0x3
	 * [7:0] SDRC_ADDR		0X0
	 */
	reset_phy(EMIF1_BASE);
	reset_phy(EMIF2_BASE);

	*((volatile int *)0x80000000) = 0;
	*((volatile int *)0x80000080) = 0;
	//*((volatile int *)0x90000000) = 0;
}
/*****************************************
 * Routine: board_init
 * Description: Early hardware init.
 *****************************************/
int board_init(void)
{
	return 0;
}

/*****************************************
 * Routine: secure_unlock
 * Description: Setup security registers for access
 * (GP Device only)
 *****************************************/
void secure_unlock_mem(void)
{
	/* Permission values for registers -Full fledged permissions to all */
	#define UNLOCK_1 0xFFFFFFFF
	#define UNLOCK_2 0x00000000
	#define UNLOCK_3 0x0000FFFF

	/* Protection Module Register Target APE (PM_RT)*/
	__raw_writel(UNLOCK_1, RT_REQ_INFO_PERMISSION_1);
	__raw_writel(UNLOCK_1, RT_READ_PERMISSION_0);
	__raw_writel(UNLOCK_1, RT_WRITE_PERMISSION_0);
	__raw_writel(UNLOCK_2, RT_ADDR_MATCH_1);

	__raw_writel(UNLOCK_3, GPMC_REQ_INFO_PERMISSION_0);
	__raw_writel(UNLOCK_3, GPMC_READ_PERMISSION_0);
	__raw_writel(UNLOCK_3, GPMC_WRITE_PERMISSION_0);

	__raw_writel(UNLOCK_3, OCM_REQ_INFO_PERMISSION_0);
	__raw_writel(UNLOCK_3, OCM_READ_PERMISSION_0);
	__raw_writel(UNLOCK_3, OCM_WRITE_PERMISSION_0);
	__raw_writel(UNLOCK_2, OCM_ADDR_MATCH_2);

	/* IVA Changes */
	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_0);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_0);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_0);

	__raw_writel(UNLOCK_1, SMS_RG_ATT0); /* SDRC region 0 public */
}

/**********************************************************
 * Routine: try_unlock_sram()
 * Description: If chip is GP/EMU(special) type, unlock the SRAM for
 *  general use.
 ***********************************************************/
void try_unlock_memory(void)
{
	int mode;

	/* if GP device unlock device SRAM for general use */
	/* secure code breaks for Secure/Emulation device - HS/E/T*/
	return;
}


#if defined(CONFIG_MPU_600) || defined(CONFIG_MPU_1000)
static scale_vcores(void)
{
	unsigned int rev = omap_revision();
	/* For VC bypass only VCOREx_CGF_FORCE  is necessary and
	 * VCOREx_CFG_VOLTAGE  changes can be discarded
	 */
	/* PRM_VC_CFG_I2C_MODE */
	*(volatile int*)(0x4A307BA8) = 0x0;
	/* PRM_VC_CFG_I2C_CLK */
	*(volatile int*)(0x4A307BAC) = 0x6026;

	/* set VCORE1 force VSEL */
	/* PRM_VC_VAL_BYPASS) */
        if(rev == OMAP4430_ES1_0)
		*(volatile int*)(0x4A307BA0) = 0x3B5512;
	else
		*(volatile int*)(0x4A307BA0) = 0x3A5512;
	*(volatile int*)(0x4A307BA0) |= 0x1000000;
	while((*(volatile int*)(0x4A307BA0)) & 0x1000000);

	/* PRM_IRQSTATUS_MPU */
	*(volatile int*)(0x4A306010) = *(volatile int*)(0x4A306010);


	/* FIXME: set VCORE2 force VSEL, Check the reset value */
	/* PRM_VC_VAL_BYPASS) */
        if(rev == OMAP4430_ES1_0)
		*(volatile int*)(0x4A307BA0) = 0x315B12;
	else
		*(volatile int*)(0x4A307BA0) = 0x295B12;
	*(volatile int*)(0x4A307BA0) |= 0x1000000;
	while((*(volatile int*)(0x4A307BA0)) & 0x1000000);

	/* PRM_IRQSTATUS_MPU */
	*(volatile int*)(0x4A306010) = *(volatile int*)(0x4A306010);

	/*/set VCORE3 force VSEL */
	/* PRM_VC_VAL_BYPASS */
        if(rev == OMAP4430_ES1_0)
		*(volatile int*)(0x4A307BA0) = 0x316112;
	else
		*(volatile int*)(0x4A307BA0) = 0x296112;
	*(volatile int*)(0x4A307BA0) |= 0x1000000;
	while((*(volatile int*)(0x4A307BA0)) & 0x1000000);

	/* PRM_IRQSTATUS_MPU */
	*(volatile int*)(0x4A306010) = *(volatile int*)(0x4A306010);

}
#endif

/**********************************************************
 * Routine: s_init
 * Description: Does early system init of muxing and clocks.
 * - Called path is with SRAM stack.
 **********************************************************/

void s_init(void)
{
	set_muxconf_regs();
	delay(100);

	/* Writing to AuxCR in U-boot using SMI for GP/EMU DEV */
	/* Currently SMI in Kernel on ES2 devices seems to have an isse
	 * Once that is resolved, we can postpone this config to kernel
	 */
	//setup_auxcr(get_device_type(), external_boot);

	ddr_init();

/* Set VCORE1 = 1.3 V, VCORE2 = VCORE3 = 1.21V */
#if defined(CONFIG_MPU_600) || defined(CONFIG_MPU_1000)
	scale_vcores();
#endif	
	prcm_init();

#ifdef CFG_DDR_TEST
	serial_init();
	ddr_test();
#endif /* CFG_DDR_TEST */

}

/*******************************************************
 * Routine: misc_init_r
 * Description: Init ethernet (done here so udelay works)
 ********************************************************/
int misc_init_r(void)
{
	return 0;
}

/******************************************************
 * Routine: wait_for_command_complete
 * Description: Wait for posting to finish on watchdog
 ******************************************************/
void wait_for_command_complete(unsigned int wd_base)
{
	int pending = 1;
	do {
		pending = __raw_readl(wd_base + WWPS);
	} while (pending);
}

/*******************************************************************
 * Routine:ether_init
 * Description: take the Ethernet controller out of reset and wait
 *  		   for the EEPROM load to complete.
 ******************************************************************/

/**********************************************
 * Routine: dram_init
 * Description: sets uboots idea of sdram size
 **********************************************/
int dram_init(void)
{
	return 0;
}

#define		OMAP44XX_WKUP_CTRL_BASE		0x4A31E000 
#if 1
#define M0_SAFE M0
#define M1_SAFE M1
#define M2_SAFE M2
#define M4_SAFE M4
#define M7_SAFE M7
#define M3_SAFE M3
#define M5_SAFE M5
#define M6_SAFE M6
#else
#define M0_SAFE M7
#define M1_SAFE M7
#define M2_SAFE M7
#define M4_SAFE M7
#define M7_SAFE M7
#define M3_SAFE M7
#define M5_SAFE M7
#define M6_SAFE M7
#endif
#define		MV(OFFSET, VALUE)\
			__raw_writew((VALUE), OMAP44XX_CTRL_BASE + (OFFSET));
#define		MV1(OFFSET, VALUE)\
			__raw_writew((VALUE), OMAP44XX_WKUP_CTRL_BASE + (OFFSET));

#define		CP(x)	(CONTROL_PADCONF_##x)
#define		WK(x)	(CONTROL_WKUP_##x)
/*
 * IEN  - Input Enable
 * IDIS - Input Disable
 * PTD  - Pull type Down
 * PTU  - Pull type Up
 * DIS  - Pull type selection is inactive
 * EN   - Pull type selection is active
 * M0   - Mode 0
 * The commented string gives the final mux configuration for that pin
 */

#define MUX_DEFAULT_OMAP4() \
	MV(CP(GPMC_AD0) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1))  /* sdmmc2_dat0 */ \
	MV(CP(GPMC_AD1) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1))  /* sdmmc2_dat1 */ \
	MV(CP(GPMC_AD2) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1))  /* sdmmc2_dat2 */ \
	MV(CP(GPMC_AD3) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1))  /* sdmmc2_dat3 */ \
	MV(CP(GPMC_AD4) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1))  /* sdmmc2_dat4 */ \
	MV(CP(GPMC_AD5) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1))  /* sdmmc2_dat5 */ \
	MV(CP(GPMC_AD6) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1))  /* sdmmc2_dat6 */ \
	MV(CP(GPMC_AD7) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1))  /* sdmmc2_dat7 */ \
	MV(CP(GPMC_AD8) , ( M3 ))  /* gpio_32, proximity sensor out to OMAP, not connected */ \
	MV(CP(GPMC_AD9) , ( M3 ))  /* gpio_33, accelerometers U14 INT to OMAP, not connected */ \
	MV(CP(GPMC_AD10) , ( OFF_EN | OFF_OUT | OFF_OUT_PTU | M3 )) /* gpio_34, Touch Panel B reset */ \
	MV(CP(GPMC_AD11) , ( M3 )) /* gpio_35, MDM_PWRON control signal to PCIE mini J3, not connected */ \
	MV(CP(GPMC_AD12) , ( M3 )) /* gpio_36, MDM_RST control signal to PCIE mini J3, not connected */ \
	MV(CP(GPMC_AD13) , ( M3 )) /* gpio_37, MDM_RST drive by MDM, not connected */ \
	MV(CP(GPMC_AD14) , PTD | OFF_EN | OFF_PD | OFF_OUT_PTD | M3) /* gpio_38, shutdown signal to camera module */ \
	MV(CP(GPMC_AD15) , ( M7 )) /* unused */ \
	MV(CP(GPMC_A16) , ( M3 ))  /* gpio_40, control signal to SD_CLK_SELECT, not connected */ \
	MV(CP(GPMC_A17) , ( PTD | M3))  /* gpio_41 */ \
	MV(CP(GPMC_A18) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_42, reset button */ \
	MV(CP(GPMC_A19) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_43, BOOT SWITCH */ \
	MV(CP(GPMC_A20) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_44, LSM303DLH INT1 output */ \
	MV(CP(GPMC_A21) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_45, LSM303DLH INT2 output*/ \
	MV(CP(GPMC_A22) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_46, LSM303DLH DRDY_M output */ \
	MV(CP(GPMC_A23) , ( M3 ))  /* gpio_47, drive power enable for Camera AF_2V8/MIPI 2V8, not connected */ \
	MV(CP(GPMC_A24) , ( M3 ))  /* gpio_48, enable VPROX power switch U13 */ \
	MV(CP(GPMC_A25) , ( PTD | M3))  /* gpio_49 */ \
	MV(CP(GPMC_NCS0) , ( M3))  /* gpio_50 */ \
	MV(CP(GPMC_NCS1) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_51, INT signal from Ambient Light & PROX sensor */ \
	MV(CP(GPMC_NCS2) , ( IEN | M3))  /* gpio_52, Host Wake signal for BT WL1273. */ \
	MV(CP(GPMC_NCS3) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_53, WL1273 WLAN IRQ */ \
	MV(CP(GPMC_NWP) , ( M3))  /* gpio_54 */ \
	MV(CP(GPMC_CLK) , ( M3))  /* gpio_55, I/O I2C bus repeater EN (H) */ \
	MV(CP(GPMC_NADV_ALE) , ( M3))  /* gpio_56, drive LED, active low */ \
	MV(CP(GPMC_NOE) , ( PTU | IEN | OFF_EN | OFF_OUT_PTD | M1))  /* sdmmc2_clk */ \
	MV(CP(GPMC_NWE) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1))  /* sdmmc2_cmd */ \
	MV(CP(GPMC_NBE0_CLE) , ( M3))  /* gpio_59, Camera Flash Enable */ \
	MV(CP(GPMC_NBE1) , ( PTD | M3))  /* gpio_60 */ \
	MV(CP(GPMC_WAIT0) , ( M3))  /* gpio_61, Microphone Amp U6 Enable */ \
	MV(CP(GPMC_WAIT1) , ( M3))  /* gpio_62, Summit Charging IC OTG_LBR */ \
	MV(CP(C2C_DATA11) , ( PTD | M3))  /* gpio_100, Summit Charging IC USB5_1_HC */ \
	MV(CP(C2C_DATA12) , ( M7))  /* unused */ \
	MV(CP(C2C_DATA13) , ( M3))  /* gpio_102, Disp0_RSTX */ \
	MV(CP(C2C_DATA14) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_103, SMT_INT edge */ \
	MV(CP(C2C_DATA15) , ( OFF_EN | OFF_OUT | OFF_OUT_PTU | M3))  /* gpio_104, OLED DISP1 ON, not connected */ \
	MV(CP(HDMI_HPD) , ( M0))  /* hdmi_hpd */ \
	MV(CP(HDMI_CEC) , ( M0))  /* hdmi_cec */ \
	MV(CP(HDMI_DDC_SCL) , ( PTU | M0))  /* hdmi_ddc_scl */ \
	MV(CP(HDMI_DDC_SDA) , ( PTU | IEN | M0))  /* hdmi_ddc_sda */ \
	MV(CP(CSI21_DX0) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpi_67, reserved for hinge detection */ \
	MV(CP(CSI21_DY0) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpi_68, reserved for hinge detection */ \
	MV(CP(CSI21_DX1) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpi_69, reserved for hinge detection */ \
	MV(CP(CSI21_DY1) , ( PTU | IEN | OFF_EN | OFF_PU | OFF_IN | M3))  /* gpi_70, TOUCH A MISO detection */ \
	MV(CP(CSI21_DX2) , ( PTU | IEN | OFF_EN | OFF_PU | OFF_IN | M3))  /* gpi_71, TOUCH1 A ATTN */ \
	MV(CP(CSI21_DY2) , ( PTU | IEN | OFF_EN | OFF_PU | OFF_IN | M3))  /* gpi_72, TOUCH1 B ATTN */ \
	MV(CP(CSI21_DX3) , ( PTU | IEN | OFF_EN | OFF_PU | OFF_IN | M3))  /* gpi_73, TOUCH B MISO detection */ \
	MV(CP(CSI21_DY3) , ( M7))  /* gpi_74, unused */ \
	MV(CP(CSI21_DX4) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpi_75, from hall effect sensor U6908 output */ \
	MV(CP(CSI21_DY4) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpi_76, from hall effect sensor U6909 output */ \
	MV(CP(CSI22_DX0) , ( IEN | M0))  /* csi22_dx0 */ \
	MV(CP(CSI22_DY0) , ( IEN | M0))  /* csi22_dy0 */ \
	MV(CP(CSI22_DX1) , ( IEN | M0))  /* csi22_dx1 */ \
	MV(CP(CSI22_DY1) , ( IEN | M0))  /* csi22_dy1 */ \
	MV(CP(CAM_SHUTTER) , ( M7))  /* unused */ \
	MV(CP(CAM_STROBE) , ( M3))  /* gpio_82, Camera flash->reset */ \
	MV(CP(CAM_GLOBALRESET) , ( M3))  /* gpio_83, Camera reset */ \
	MV(CP(USBB1_ULPITLL_CLK) , ( M7))  /* Reserved for HSI V2 silicon (CAWAKE) */ \
	MV(CP(USBB1_ULPITLL_STP) , ( M7))  /* Reserved for HSI V2 silicon (CADATA) */ \
	MV(CP(USBB1_ULPITLL_DIR) , ( M7))  /* Reserved for HSI V2 silicon (CAFLAG) */ \
	MV(CP(USBB1_ULPITLL_NXT) , ( M7))  /* Reserved for HSI V2 silicon (ACREADY) */ \
	MV(CP(USBB1_ULPITLL_DAT0) , ( M7))  /* Reserved for HSI V2 silicon (ACWAKE) */ \
	MV(CP(USBB1_ULPITLL_DAT1) , ( M7))  /* Reserved for HSI V2 silicon (ACDATA) */ \
	MV(CP(USBB1_ULPITLL_DAT2) , ( M7))  /* Reserved for HSI V2 silicon (ACFLAG) */ \
	MV(CP(USBB1_ULPITLL_DAT3) , ( M7))  /* Reserved for HSI V2 silicon (CAREADY) */ \
	MV(CP(USBB1_ULPITLL_DAT4) , ( M3))  /* gpio_92, service request to DB5700 */ \
	MV(CP(USBB1_ULPITLL_DAT5) , ( M3))  /* gpio_93, drive U26 OLED1 PS ENABLE */ \
	MV(CP(USBB1_ULPITLL_DAT6) , ( M3))  /* gpio_94, drive U28 OLED2 PS ENABLE */ \
	MV(CP(USBB1_ULPITLL_DAT7) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_95, resout2n from DB5700 */ \
	MV(CP(USBB1_HSIC_DATA) , ( M7))  /* currently unused */ \
	MV(CP(USBB1_HSIC_STROBE) , ( M7))  /* currently unused */ \
	MV(CP(USBC1_ICUSB_DP) , ( IEN | M0))  /* usbc1_icusb_dp */ \
	MV(CP(USBC1_ICUSB_DM) , ( IEN | M0))  /* usbc1_icusb_dm */ \
	MV(CP(SDMMC1_CLK) , ( PTU | OFF_EN | OFF_OUT_PTD | M0))  /* sdmmc1_clk */ \
	MV(CP(SDMMC1_CMD) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* sdmmc1_cmd */ \
	MV(CP(SDMMC1_DAT0) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* sdmmc1_dat0 */ \
	MV(CP(SDMMC1_DAT1) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* sdmmc1_dat1 */ \
	MV(CP(SDMMC1_DAT2) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* sdmmc1_dat2 */ \
	MV(CP(SDMMC1_DAT3) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* sdmmc1_dat3 */ \
	MV(CP(SDMMC1_DAT4) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* sdmmc1_dat4 */ \
	MV(CP(SDMMC1_DAT5) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* sdmmc1_dat5 */ \
	MV(CP(SDMMC1_DAT6) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* sdmmc1_dat6 */ \
	MV(CP(SDMMC1_DAT7) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* sdmmc1_dat7 */ \
	MV(CP(ABE_MCBSP2_CLKX) , ( IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* abe_mcbsp2_clkx */ \
	MV(CP(ABE_MCBSP2_DR) , ( IEN | OFF_EN | OFF_OUT_PTD | M0))  /* abe_mcbsp2_dr */ \
	MV(CP(ABE_MCBSP2_DX) , ( OFF_EN | OFF_OUT_PTD | M0))  /* abe_mcbsp2_dx */ \
	MV(CP(ABE_MCBSP2_FSX) , ( IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* abe_mcbsp2_fsx */ \
	MV(CP(ABE_MCBSP1_CLKX) , ( IEN | M1))  /* abe_slimbus1_clock */ \
	MV(CP(ABE_MCBSP1_DR) , ( IEN | M1))  /* abe_slimbus1_data */ \
	MV(CP(ABE_MCBSP1_DX) , ( OFF_EN | OFF_OUT_PTD | M0))  /* abe_mcbsp1_dx */ \
	MV(CP(ABE_MCBSP1_FSX) , ( IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* abe_mcbsp1_fsx */ \
	MV(CP(ABE_PDM_UL_DATA) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* abe_pdm_ul_data */ \
	MV(CP(ABE_PDM_DL_DATA) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* abe_pdm_dl_data */ \
	MV(CP(ABE_PDM_FRAME) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* abe_pdm_frame */ \
	MV(CP(ABE_PDM_LB_CLK) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* abe_pdm_lb_clk */ \
	MV(CP(ABE_CLKS) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* abe_clks */ \
	MV(CP(ABE_DMIC_CLK1) , ( M0))  /* abe_dmic_clk1 */ \
	MV(CP(ABE_DMIC_DIN1) , ( IEN | M0))  /* abe_dmic_din1 */ \
	MV(CP(ABE_DMIC_DIN2),   ( M7)) /* unused */ \
	MV(CP(ABE_DMIC_DIN3),   ( M7)) /* unused */ \
	MV(CP(UART2_CTS) , ( PTU | IEN | M0))  /* uart2_cts */ \
	MV(CP(UART2_RTS) , ( M0))  /* uart2_rts */ \
	MV(CP(UART2_RX) , ( PTU | IEN | M0))  /* uart2_rx */ \
	MV(CP(UART2_TX) , ( M0))  /* uart2_tx */ \
	MV(CP(HDQ_SIO) , ( M3))  /* gpio_127 */ \
	MV(CP(I2C1_SCL) , ( PTU | IEN | M0))  /* i2c1_scl */ \
	MV(CP(I2C1_SDA) , ( PTU | IEN | M0))  /* i2c1_sda */ \
	MV(CP(I2C2_SCL) , ( PTU | IEN | M0))  /* i2c2_scl */ \
	MV(CP(I2C2_SDA) , ( PTU | IEN | M0))  /* i2c2_sda */ \
	MV(CP(I2C3_SCL) , ( PTU | IEN | M0))  /* i2c3_scl */ \
	MV(CP(I2C3_SDA) , ( PTU | IEN | M0))  /* i2c3_sda */ \
	MV(CP(I2C4_SCL) , ( PTU | IEN | M0))  /* i2c4_scl */ \
	MV(CP(I2C4_SDA) , ( PTU | IEN | M0))  /* i2c4_sda */ \
	MV(CP(MCSPI1_CLK) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* mcspi1_clk */ \
	MV(CP(MCSPI1_SOMI) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* mcspi1_somi */ \
	MV(CP(MCSPI1_SIMO) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* mcspi1_simo */ \
	MV(CP(MCSPI1_CS0) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* mcspi1_cs0, MPIP OLED 1 CS */ \
	MV(CP(MCSPI1_CS1) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* mcspi1_cs1, touch CS A */ \
	MV(CP(MCSPI1_CS2) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi1_cs2, touch CS B */ \
	MV(CP(MCSPI1_CS3) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi1_cs3, RGB OLED 2 CS  */ \
	MV(CP(UART3_CTS_RCTX) , (M3)) /* gpio_141, eMMC 2V8 EN */ \
	MV(CP(UART3_RTS_SD) , (M7)) /* unused */ \
	MV(CP(UART3_RX_IRRX) , ( IEN | M0))  /* uart3_rx */ \
	MV(CP(UART3_TX_IRTX) , ( M0))  /* uart3_tx */ \
	MV(CP(SDMMC5_CLK) , ( PTU | IEN | OFF_EN | OFF_OUT_PTD | M0))  /* sdmmc5_clk */ \
	MV(CP(SDMMC5_CMD) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* sdmmc5_cmd */ \
	MV(CP(SDMMC5_DAT0) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* sdmmc5_dat0 */ \
	MV(CP(SDMMC5_DAT1) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* sdmmc5_dat1 */ \
	MV(CP(SDMMC5_DAT2) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* sdmmc5_dat2 */ \
	MV(CP(SDMMC5_DAT3) , ( PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* sdmmc5_dat3 */ \
	MV(CP(MCSPI4_CLK) , ( IEN |  OFF_PD | OFF_IN | M7))  /* safe_mode, mcspi4_clk */ \
	MV(CP(MCSPI4_SIMO) , ( IEN | OFF_PD | OFF_IN | M7))  /* safe_mode, mcspi4_simo */ \
	MV(CP(MCSPI4_SOMI) , ( IEN | OFF_PD | OFF_IN | M7))  /* safe_mode, mcspi4_somi */ \
	MV(CP(MCSPI4_CS0) , ( IEN | OFF_EN | OFF_PD | OFF_IN | M7))  /* safe_mode, mcspi4_cs0 */ \
	MV(CP(UART4_RX) , ( IEN | M0))  /* uart4_rx */ \
	MV(CP(UART4_TX) , ( M0))  /* uart4_tx */ \
	MV(CP(USBB2_ULPITLL_CLK) , ( OFF_EN | OFF_OUT | OFF_OUT_PTU | M3)) /* gpio_157, Touch Panel A reset */ \
	MV(CP(USBB2_ULPITLL_STP) , ( IEN | M5))  /* dispc2_data23 */ \
	MV(CP(USBB2_ULPITLL_DIR) , ( IEN | M5))  /* dispc2_data22 */ \
	MV(CP(USBB2_ULPITLL_NXT) , ( IEN | M5))  /* dispc2_data21 */ \
	MV(CP(USBB2_ULPITLL_DAT0) , ( IEN | M5))  /* dispc2_data20 */ \
	MV(CP(USBB2_ULPITLL_DAT1) , ( IEN | M5))  /* dispc2_data19 */ \
	MV(CP(USBB2_ULPITLL_DAT2) , ( IEN | M5))  /* dispc2_data18 */ \
	MV(CP(USBB2_ULPITLL_DAT3) , ( IEN | M5))  /* dispc2_data15 */ \
	MV(CP(USBB2_ULPITLL_DAT4) , ( IEN | M5))  /* dispc2_data14 */ \
	MV(CP(USBB2_ULPITLL_DAT5) , ( IEN | M5))  /* dispc2_data13 */ \
	MV(CP(USBB2_ULPITLL_DAT6) , ( IEN | M5))  /* dispc2_data12 */ \
	MV(CP(USBB2_ULPITLL_DAT7) , ( IEN | M5))  /* dispc2_data11 */ \
	MV(CP(USBB2_HSIC_DATA), (M7)) /* unused */ \
	MV(CP(USBB2_HSIC_STROBE),       (M7)) /* unused */ \
	MV(CP(UNIPRO_TX0) , (OFF_EN | OFF_PD | OFF_IN | M1))  /* kpd_col0 */ \
	MV(CP(UNIPRO_TY0) , (OFF_EN | OFF_PD | OFF_IN | M1))  /* kpd_col1 */ \
	MV(CP(UNIPRO_TX1) , (OFF_EN | OFF_PD | OFF_IN | M1))  /* kpd_col2 */ \
	MV(CP(UNIPRO_TY1) , (OFF_EN | OFF_OUT | OFF_OUT_PTU | M3)) /* gpio_174, SPI4_CS_SEL_TOUCH2 gate */ \
	MV(CP(UNIPRO_TX2) , (OFF_EN | OFF_OUT | OFF_OUT_PTU | M3)) /*  'gpio_0, SPI4_CS_SEL_DISP2 gate' */ \
	MV(CP(UNIPRO_TY2) , (M7)) /* unused, reserved for EL_ON_2 */ \
	MV(CP(UNIPRO_RX0) , (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_row0 */ \
	MV(CP(UNIPRO_RY0) , (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_row1 */ \
	MV(CP(UNIPRO_RX1) , (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_row2 */ \
	MV(CP(UNIPRO_RY1) , (M7)) /* unused */ \
	MV(CP(UNIPRO_RX2) , (M7)) /* unused */ \
	MV(CP(UNIPRO_RY2) , (M7)) /* unused */ \
	MV(CP(USBA0_OTG_CE) , ( PTD | OFF_EN | OFF_PD | OFF_OUT_PTD | M0))  /* usba0_otg_ce */ \
	MV(CP(USBA0_OTG_DP) , ( IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* usba0_otg_dp */ \
	MV(CP(USBA0_OTG_DM) , ( IEN | OFF_EN | OFF_PD | OFF_IN | M0))  /* usba0_otg_dm */ \
	MV(CP(FREF_CLK1_OUT),   (M7)) /* unused */ \
	MV(CP(FREF_CLK2_OUT) , ( M0))  /* fref_clk2_out */ \
	MV(CP(SYS_NIRQ1) , ( PTU | IEN | M0))  /* sys_nirq1 */ \
	MV(CP(SYS_NIRQ2) , ( PTU | IEN | M0))  /* sys_nirq2 */ \
	MV(CP(SYS_BOOT0) , (M0)) /* SYS BOOT SWITCH */ \
	MV(CP(SYS_BOOT1) , (M0)) /* SYS BOOT SWITCH */ \
	MV(CP(SYS_BOOT2) , (M0)) /* SYS BOOT SWITCH */ \
	MV(CP(SYS_BOOT3) , (M0)) /* SYS BOOT SWITCH */ \
	MV(CP(SYS_BOOT4) , (M0)) /* SYS BOOT SWITCH */ \
	MV(CP(SYS_BOOT5) , (M0)) /* SYS BOOT SWITCH */ \
	MV(CP(DPM_EMU0) , ( PTD | OFF_EN | OFF_PD | OFF_OUT_PTD | M0))  /* dpm_emu0, MIPI debug */ \
	MV(CP(DPM_EMU1) , ( M3))  /* gpio_12, J33_OLED1_RESET */ \
	MV(CP(DPM_EMU2) , ( M3))  /* gpio_13, J30_OLED2_RESET */ \
	MV(CP(DPM_EMU3) , ( IEN | M5))  /* dispc2_data10 */ \
	MV(CP(DPM_EMU4) , ( IEN | M5))  /* dispc2_data9 */ \
	MV(CP(DPM_EMU5) , ( IEN | M5))  /* dispc2_data16 */ \
	MV(CP(DPM_EMU6) , ( IEN | M5))  /* dispc2_data17 */ \
	MV(CP(DPM_EMU7) , ( IEN | M5))  /* dispc2_hsync */ \
	MV(CP(DPM_EMU8) , ( IEN | M5))  /* dispc2_pclk */ \
	MV(CP(DPM_EMU9) , ( IEN | M5))  /* dispc2_vsync */ \
	MV(CP(DPM_EMU10) , ( IEN | M5))  /* dispc2_de */ \
	MV(CP(DPM_EMU11) , ( IEN | M5))  /* dispc2_data8 */ \
	MV(CP(DPM_EMU12) , ( IEN | M5))  /* dispc2_data7 */ \
	MV(CP(DPM_EMU13) , ( IEN | M5))  /* dispc2_data6 */ \
	MV(CP(DPM_EMU14) , ( IEN | M5))  /* dispc2_data5 */ \
	MV(CP(DPM_EMU15) , ( IEN | M5))  /* dispc2_data4 */ \
	MV(CP(DPM_EMU16) , ( IEN | M5))  /* dispc2_data3 */ \
	MV(CP(DPM_EMU17) , ( IEN | M5))  /* dispc2_data2 */ \
	MV(CP(DPM_EMU18) , ( IEN | M5))  /* dispc2_data1 */ \
	MV(CP(DPM_EMU19) , ( IEN | M5))  /* dispc2_data0 */ \
	MV(CP(SYS_BOOT5) , ( M3 )) /* gpio 189 */ \
	MV1(WK(PAD0_SIM_IO) , ( M3))  /* gpio_wk0, MDM WAKEIN DB5700 */ \
	MV1(WK(PAD1_SIM_CLK) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_wk1, DOCK_DET_WAKE->HDMI HPD INT */ \
	MV1(WK(PAD0_SIM_RESET) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_wk2, BUTTON_WAKE */ \
	MV1(WK(PAD1_SIM_CD) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_wk3, Lid switch wake signal */ \
	MV1(WK(PAD0_SIM_PWRCTRL) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_wk4, Lid switch wake signal */ \
	MV1(WK(PAD1_SR_SCL) , ( PTU | IEN | M0))  /* sr_scl */ \
	MV1(WK(PAD0_SR_SDA) , ( PTU | IEN | M0))  /* sr_sda */ \
	MV1(WK(PAD1_FREF_XTAL_IN) , ( M0))  /* # */ \
	MV1(WK(PAD0_FREF_SLICER_IN) , ( M0))  /* fref_slicer_in */ \
	MV1(WK(PAD1_FREF_CLK_IOREQ) , ( M0))  /* fref_clk_ioreq */ \
	MV1(WK(PAD0_FREF_CLK0_OUT) , ( M2))  /* sys_drm_msecure */ \
	MV1(WK(PAD1_FREF_CLK3_REQ),     (M7)) /* unused, reserved for EL_ON_1 control */ \
	MV1(WK(PAD0_FREF_CLK3_OUT),     (M7)) /* unused */ \
	MV1(WK(PAD1_FREF_CLK4_REQ),     (M0_SAFE)) /* unused, don't support M7 in manual */ \
	MV1(WK(PAD0_FREF_CLK4_OUT),     (M0_SAFE)) /* unused, don't support M7 in manual */ \
	MV1(WK(PAD1_SYS_32K) , ( IEN | M0))  /* sys_32k */ \
	MV1(WK(PAD0_SYS_NRESPWRON) , ( M0))  /* sys_nrespwron */ \
	MV1(WK(PAD1_SYS_NRESWARM) , ( M0))  /* sys_nreswarm */ \
	MV1(WK(PAD0_SYS_PWR_REQ) , ( PTU | M0))  /* sys_pwr_req */ \
	MV1(WK(PAD1_SYS_PWRON_RESET) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_wk29, PMIC INT */ \
	MV1(WK(PAD0_SYS_BOOT6) , ( PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3))  /* gpio_wk9, MDM_RST_IN */ \
	MV1(WK(PAD1_SYS_BOOT7) , ( M0))  /* SYS BOOT SWITCH */ \
//	MV1(WK(PAD0_JTAG_NTRST) , ( IEN | M0))  /* jtag_ntrst */ \
	MV1(WK(PAD1_JTAG_TCK) , ( IEN | M0))  /* jtag_tck */ \
	MV1(WK(PAD0_JTAG_RTCK) , ( M0))  /* jtag_rtck */ \
	MV1(WK(PAD1_JTAG_TMS_TMSC) , ( IEN | M0))  /* jtag_tms_tmsc */ \
	MV1(WK(PAD0_JTAG_TDI) , ( IEN | M0))  /* jtag_tdi */ \
	MV1(WK(PAD1_JTAG_TDO) , ( M0))  /* jtag_tdo */
 
#define MUX_DEFAULT_OMAP4_ALL() \
  	MV(CP(GPMC_AD0),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* sdmmc2_dat0 */ \
	MV(CP(GPMC_AD1),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* sdmmc2_dat1 */ \
	MV(CP(GPMC_AD2),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* sdmmc2_dat2 */ \
	MV(CP(GPMC_AD3),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* sdmmc2_dat3 */ \
	MV(CP(GPMC_AD4),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* sdmmc2_dat4 */ \
	MV(CP(GPMC_AD5),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* sdmmc2_dat5 */ \
	MV(CP(GPMC_AD6),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* sdmmc2_dat6 */ \
	MV(CP(GPMC_AD7),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* sdmmc2_dat7 */ \
	MV(CP(GPMC_AD8),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3)) /* gpio_32 */ \
	MV(CP(GPMC_AD9),	(M3_SAFE)) /* gpio_33 */ \
	MV(CP(GPMC_AD10),	(M3_SAFE)) /* gpio_34 */ \
	MV(CP(GPMC_AD11),	(M3_SAFE)) /* gpio_35 */ \
	MV(CP(GPMC_AD12),	(M3_SAFE)) /* gpio_36 */ \
	MV(CP(GPMC_AD13),	(PTD | OFF_EN | OFF_PD | OFF_OUT_PTD | M3)) /* gpio_37 */ \
	MV(CP(GPMC_AD14),	(PTD | OFF_EN | OFF_PD | OFF_OUT_PTD | M3)) /* gpio_38 */ \
	MV(CP(GPMC_AD15),	(PTD | OFF_EN | OFF_PD | OFF_OUT_PTD | M3)) /* gpio_39 */ \
	MV(CP(GPMC_A16),	(M3_SAFE)) /* gpio_40 */ \
	MV(CP(GPMC_A17),	(M3_SAFE)) /* gpio_41 */ \
	MV(CP(GPMC_A18),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_row6 */ \
	MV(CP(GPMC_A19),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_row7 */ \
	MV(CP(GPMC_A20),	(M3_SAFE)) /* gpio_44 */ \
	MV(CP(GPMC_A21),	(M3_SAFE)) /* gpio_45 */ \
	MV(CP(GPMC_A22),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_col6 */ \
	MV(CP(GPMC_A23),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_col7 */ \
	MV(CP(GPMC_A24),	(M3_SAFE)) /* gpio_48 */ \
	MV(CP(GPMC_A25),	(M3_SAFE)) /* gpio_49 */ \
	MV(CP(GPMC_NCS0),	(M0)) /* gpmc_ncs0 */ \
	MV(CP(GPMC_NCS1),	(M3_SAFE)) /* gpio_51 */ \
	MV(CP(GPMC_NCS2),	(M3_SAFE)) /* gpio_52 */ \
	MV(CP(GPMC_NCS3),	(M3_SAFE)) /* gpio_53 */ \
	MV(CP(GPMC_NWP),	(M0_SAFE)) /* gpmc_nwp */ \
	MV(CP(GPMC_CLK),	(M3_SAFE)) /* gpio_55 */ \
	MV(CP(GPMC_NADV_ALE),	(M0)) /* gpmc_nadv_ale */ \
	MV(CP(GPMC_NOE),	(PTU | OFF_EN | OFF_OUT_PTD | M1)) /* sdmmc2_clk */ \
	MV(CP(GPMC_NWE),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* sdmmc2_cmd */ \
	MV(CP(GPMC_NBE0_CLE),	(M0)) /* gpmc_nbe0_cle*/ \
	MV(CP(GPMC_NBE1),	(M3_SAFE)) /* gpio_60 */ \
	MV(CP(GPMC_WAIT0),	(M0)) /* gpmc_wait */ \
	MV(CP(GPMC_WAIT1),	(M3_SAFE)) /* gpio_62 */ \
	MV(CP(C2C_DATA11),	(M3_SAFE)) /* gpio_100 */ \
	MV(CP(C2C_DATA12),	(M1_SAFE)) /* dsi1_te0 */ \
	MV(CP(C2C_DATA13),	(M3_SAFE)) /* gpio_102 */ \
	MV(CP(C2C_DATA14),	(M1_SAFE)) /* dsi2_te0 */ \
	MV(CP(C2C_DATA15),	(M3_SAFE)) /* gpio_104 */ \
	MV(CP(HDMI_HPD),	(M0_SAFE)) /* hdmi_hpd */ \
	MV(CP(HDMI_CEC),	(M0_SAFE)) /* hdmi_cec */ \
	MV(CP(HDMI_DDC_SCL),	(M0_SAFE)) /* hdmi_ddc_scl */ \
	MV(CP(HDMI_DDC_SDA),	(M0_SAFE)) /* hdmi_ddc_sda */ \
	MV(CP(CSI21_DX0),	(M0_SAFE)) /* csi21_dx0 */ \
	MV(CP(CSI21_DY0),	(M0_SAFE)) /* csi21_dy0 */ \
	MV(CP(CSI21_DX1),	(M0_SAFE)) /* csi21_dx1 */ \
	MV(CP(CSI21_DY1),	(M0_SAFE)) /* csi21_dy1 */ \
	MV(CP(CSI21_DX2),	(M0_SAFE)) /* csi21_dx2 */ \
	MV(CP(CSI21_DY2),	(M0_SAFE)) /* csi21_dy2 */ \
	MV(CP(CSI21_DX3),	(M0_SAFE)) /* csi21_dx3 */ \
	MV(CP(CSI21_DY3),	(M0_SAFE)) /* csi21_dy3 */ \
	MV(CP(CSI21_DX4),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3)) /* gpi_75 */ \
	MV(CP(CSI21_DY4),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M3)) /* gpi_76 */ \
	MV(CP(CSI22_DX0),	(M0_SAFE)) /* csi22_dx0 */ \
	MV(CP(CSI22_DY0),	(M0_SAFE)) /* csi22_dy0 */ \
	MV(CP(CSI22_DX1),	(M0_SAFE)) /* csi22_dx1 */ \
	MV(CP(CSI22_DY1),	(M0_SAFE)) /* csi22_dy1 */ \
	MV(CP(CAM_SHUTTER),	(PTD | OFF_EN | OFF_PD | OFF_OUT_PTD | M0)) /* cam_shutter */ \
	MV(CP(CAM_STROBE),	(PTD | OFF_EN | OFF_PD | OFF_OUT_PTD | M0)) /* cam_strobe */ \
	MV(CP(CAM_GLOBALRESET),	(PTD | OFF_EN | OFF_PD | OFF_OUT_PTD | M3)) /* gpio_83 */ \
	MV(CP(USBB1_ULPITLL_CLK),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M4)) /* usbb1_ulpiphy_clk */ \
	MV(CP(USBB1_ULPITLL_STP),	(PTU | OFF_EN | OFF_OUT_PTD | M4)) /* usbb1_ulpiphy_stp */ \
	MV(CP(USBB1_ULPITLL_DIR),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M4)) /* usbb1_ulpiphy_dir */ \
	MV(CP(USBB1_ULPITLL_NXT),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M4)) /* usbb1_ulpiphy_nxt */ \
	MV(CP(USBB1_ULPITLL_DAT0),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M4)) /* usbb1_ulpiphy_dat0 */ \
	MV(CP(USBB1_ULPITLL_DAT1),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M4)) /* usbb1_ulpiphy_dat1 */ \
	MV(CP(USBB1_ULPITLL_DAT2),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M4)) /* usbb1_ulpiphy_dat2 */ \
	MV(CP(USBB1_ULPITLL_DAT3),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M4)) /* usbb1_ulpiphy_dat3 */ \
	MV(CP(USBB1_ULPITLL_DAT4),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M4)) /* usbb1_ulpiphy_dat4 */ \
	MV(CP(USBB1_ULPITLL_DAT5),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M4)) /* usbb1_ulpiphy_dat5 */ \
	MV(CP(USBB1_ULPITLL_DAT6),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M4)) /* usbb1_ulpiphy_dat6 */ \
	MV(CP(USBB1_ULPITLL_DAT7),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M4)) /* usbb1_ulpiphy_dat7 */ \
	MV(CP(USBB1_HSIC_DATA),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usbb1_hsic_data */ \
	MV(CP(USBB1_HSIC_STROBE),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usbb1_hsic_strobe */ \
	MV(CP(USBC1_ICUSB_DP),	(M0_SAFE)) /* usbc1_icusb_dp */ \
	MV(CP(USBC1_ICUSB_DM),	(M0_SAFE)) /* usbc1_icusb_dm */ \
	MV(CP(SDMMC1_CLK),	(PTU | OFF_EN | OFF_OUT_PTD | M0)) /* sdmmc1_clk */ \
	MV(CP(SDMMC1_CMD),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc1_cmd */ \
	MV(CP(SDMMC1_DAT0),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc1_dat0 */ \
	MV(CP(SDMMC1_DAT1),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc1_dat1 */ \
	MV(CP(SDMMC1_DAT2),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc1_dat2 */ \
	MV(CP(SDMMC1_DAT3),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc1_dat3 */ \
	MV(CP(SDMMC1_DAT4),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc1_dat4 */ \
	MV(CP(SDMMC1_DAT5),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc1_dat5 */ \
	MV(CP(SDMMC1_DAT6),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc1_dat6 */ \
	MV(CP(SDMMC1_DAT7),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc1_dat7 */ \
	MV(CP(ABE_MCBSP2_CLKX),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* abe_mcbsp2_clkx */ \
	MV(CP(ABE_MCBSP2_DR),	(IEN | OFF_EN | OFF_OUT_PTD | M0)) /* abe_mcbsp2_dr */ \
	MV(CP(ABE_MCBSP2_DX),	(OFF_EN | OFF_OUT_PTD | M0)) /* abe_mcbsp2_dx */ \
	MV(CP(ABE_MCBSP2_FSX),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* abe_mcbsp2_fsx */ \
	MV(CP(ABE_MCBSP1_CLKX),	(M1_SAFE)) /* abe_slimbus1_clock */ \
	MV(CP(ABE_MCBSP1_DR),	(M1_SAFE)) /* abe_slimbus1_data */ \
	MV(CP(ABE_MCBSP1_DX),	(OFF_EN | OFF_OUT_PTD | M0)) /* abe_mcbsp1_dx */ \
	MV(CP(ABE_MCBSP1_FSX),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* abe_mcbsp1_fsx */ \
	MV(CP(ABE_PDM_UL_DATA),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0_SAFE)) /* abe_pdm_ul_data */ \
	MV(CP(ABE_PDM_DL_DATA),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0_SAFE)) /* abe_pdm_dl_data */ \
	MV(CP(ABE_PDM_FRAME),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0_SAFE)) /* abe_pdm_frame */ \
	MV(CP(ABE_PDM_LB_CLK),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0_SAFE)) /* abe_pdm_lb_clk */ \
	MV(CP(ABE_CLKS),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0_SAFE)) /* abe_clks */ \
	MV(CP(ABE_DMIC_CLK1),	(M0_SAFE)) /* abe_dmic_clk1 */ \
	MV(CP(ABE_DMIC_DIN1),	(M0_SAFE)) /* abe_dmic_din1 */ \
	MV(CP(ABE_DMIC_DIN2),	(M0_SAFE)) /* abe_dmic_din2 */ \
	MV(CP(ABE_DMIC_DIN3),	(M0_SAFE)) /* abe_dmic_din3 */ \
	MV(CP(UART2_CTS),	(PTU | IEN | M0)) /* uart2_cts */ \
	MV(CP(UART2_RTS),	(M0)) /* uart2_rts */ \
	MV(CP(UART2_RX),	(PTU | IEN | M0)) /* uart2_rx */ \
	MV(CP(UART2_TX),	(M0)) /* uart2_tx */ \
	MV(CP(HDQ_SIO),	(M3_SAFE)) /* gpio_127 */ \
	MV(CP(I2C1_SCL),	(PTU | IEN | M0)) /* i2c1_scl */ \
	MV(CP(I2C1_SDA),	(PTU | IEN | M0)) /* i2c1_sda */ \
	MV(CP(I2C2_SCL),	(PTU | IEN | M0)) /* i2c2_scl */ \
	MV(CP(I2C2_SDA),	(PTU | IEN | M0)) /* i2c2_sda */ \
	MV(CP(I2C3_SCL),	(PTU | IEN | M0)) /* i2c3_scl */ \
	MV(CP(I2C3_SDA),	(PTU | IEN | M0)) /* i2c3_sda */ \
	MV(CP(I2C4_SCL),	(PTU | IEN | M0)) /* i2c4_scl */ \
	MV(CP(I2C4_SDA),	(PTU | IEN | M0)) /* i2c4_sda */ \
	MV(CP(MCSPI1_CLK),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi1_clk */ \
	MV(CP(MCSPI1_SOMI),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi1_somi */ \
	MV(CP(MCSPI1_SIMO),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi1_simo */ \
	MV(CP(MCSPI1_CS0),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi1_cs0 */ \
	MV(CP(MCSPI1_CS1),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0_SAFE)) /* mcspi1_cs1 */ \
	MV(CP(MCSPI1_CS2),	(OFF_EN | OFF_OUT_PTU | M3)) /* gpio_139 */ \
	MV(CP(MCSPI1_CS3),	(M3_SAFE)) /* gpio_140 */ \
	MV(CP(UART3_CTS_RCTX),	(PTU | IEN | M0)) /* uart3_tx */ \
	MV(CP(UART3_RTS_SD),	(M0)) /* uart3_rts_sd */ \
	MV(CP(UART3_RX_IRRX),	(IEN | M0)) /* uart3_rx */ \
	MV(CP(UART3_TX_IRTX),	(M0)) /* uart3_tx */ \
	MV(CP(SDMMC5_CLK),	(PTU | OFF_EN | OFF_OUT_PTD | M0)) /* sdmmc5_clk */ \
	MV(CP(SDMMC5_CMD),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc5_cmd */ \
	MV(CP(SDMMC5_DAT0),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc5_dat0 */ \
	MV(CP(SDMMC5_DAT1),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc5_dat1 */ \
	MV(CP(SDMMC5_DAT2),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc5_dat2 */ \
	MV(CP(SDMMC5_DAT3),	(PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* sdmmc5_dat3 */ \
	MV(CP(MCSPI4_CLK),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi4_clk */ \
	MV(CP(MCSPI4_SIMO),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi4_simo */ \
	MV(CP(MCSPI4_SOMI),	(IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi4_somi */ \
	MV(CP(MCSPI4_CS0),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* mcspi4_cs0 */ \
	MV(CP(UART4_RX),	(IEN | M0)) /* uart4_rx */ \
	MV(CP(UART4_TX),	(M0)) /* uart4_tx */ \
	MV(CP(USBB2_ULPITLL_CLK),	(M3)) /* gpio_157 */ \
	MV(CP(USBB2_ULPITLL_STP),	(M5)) /* dispc2_data23 */ \
	MV(CP(USBB2_ULPITLL_DIR),	(M5)) /* dispc2_data22 */ \
	MV(CP(USBB2_ULPITLL_NXT),	(M5)) /* dispc2_data21 */ \
	MV(CP(USBB2_ULPITLL_DAT0),	(M5)) /* dispc2_data20 */ \
	MV(CP(USBB2_ULPITLL_DAT1),	(M5)) /* dispc2_data19 */ \
	MV(CP(USBB2_ULPITLL_DAT2),	(M5)) /* dispc2_data18 */ \
	MV(CP(USBB2_ULPITLL_DAT3),	(M5)) /* dispc2_data15 */ \
	MV(CP(USBB2_ULPITLL_DAT4),	(M5)) /* dispc2_data14 */ \
	MV(CP(USBB2_ULPITLL_DAT5),	(M5)) /* dispc2_data13 */ \
	MV(CP(USBB2_ULPITLL_DAT6),	(M5)) /* dispc2_data12 */ \
	MV(CP(USBB2_ULPITLL_DAT7),	(M5)) /* dispc2_data11 */ \
	MV(CP(USBB2_HSIC_DATA),	(OFF_EN | OFF_OUT_PTU | M3)) /* gpio_169 */ \
	MV(CP(USBB2_HSIC_STROBE),	(OFF_EN | OFF_OUT_PTU | M3)) /* gpio_170 */ \
	MV(CP(UNIPRO_TX0),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_col0 */ \
	MV(CP(UNIPRO_TY0),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_col1 */ \
	MV(CP(UNIPRO_TX1),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_col2 */ \
	MV(CP(UNIPRO_TY1),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_col3 */ \
	MV(CP(UNIPRO_TX2),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_col4 */ \
	MV(CP(UNIPRO_TY2),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_col5 */ \
	MV(CP(UNIPRO_RX0),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_row0 */ \
	MV(CP(UNIPRO_RY0),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_row1 */ \
	MV(CP(UNIPRO_RX1),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_row2 */ \
	MV(CP(UNIPRO_RY1),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_row3 */ \
	MV(CP(UNIPRO_RX2),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_row4 */ \
	MV(CP(UNIPRO_RY2),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M1)) /* kpd_row5 */ \
	MV(CP(USBA0_OTG_CE),	(PTU | OFF_EN | OFF_PD | OFF_OUT_PTD | M0)) /* usba0_otg_ce */ \
	MV(CP(USBA0_OTG_DP),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usba0_otg_dp */ \
	MV(CP(USBA0_OTG_DM),	(PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)) /* usba0_otg_dm */ \
	MV(CP(FREF_CLK1_OUT),	(M0_SAFE)) /* fref_clk1_out */ \
	MV(CP(FREF_CLK2_OUT),	(M0_SAFE)) /* fref_clk2_out */ \
	MV(CP(SYS_NIRQ1),	(PTU | IEN | M0)) /* sys_nirq1 */ \
	MV(CP(SYS_NIRQ2),	(PTU | IEN | M0)) /* sys_nirq2 */ \
	MV(CP(SYS_BOOT0),	(M3_SAFE)) /* gpio_184 */ \
	MV(CP(SYS_BOOT1),	(M3_SAFE)) /* gpio_185 */ \
	MV(CP(SYS_BOOT2),	(M3_SAFE)) /* gpio_186 */ \
	MV(CP(SYS_BOOT3),	(M3_SAFE)) /* gpio_187 */ \
	MV(CP(SYS_BOOT4),	(M3_SAFE)) /* gpio_188 */ \
	MV(CP(SYS_BOOT5),	(M3_SAFE)) /* gpio_189 */ \
	MV(CP(DPM_EMU0),	(M0_SAFE)) /* dpm_emu0 */ \
	MV(CP(DPM_EMU1),	(M0_SAFE)) /* dpm_emu1 */ \
	MV(CP(DPM_EMU2),	(M0_SAFE)) /* dpm_emu2 */ \
	MV(CP(DPM_EMU3),	(M5)) /* dispc2_data10 */ \
	MV(CP(DPM_EMU4),	(M5)) /* dispc2_data9 */ \
	MV(CP(DPM_EMU5),	(M5)) /* dispc2_data16 */ \
	MV(CP(DPM_EMU6),	(M5)) /* dispc2_data17 */ \
	MV(CP(DPM_EMU7),	(M5)) /* dispc2_hsync */ \
	MV(CP(DPM_EMU8),	(M5)) /* dispc2_pclk */ \
	MV(CP(DPM_EMU9),	(M5)) /* dispc2_vsync */ \
	MV(CP(DPM_EMU10),	(M5)) /* dispc2_de */ \
	MV(CP(DPM_EMU11),	(M5)) /* dispc2_data8 */ \
	MV(CP(DPM_EMU12),	(M5)) /* dispc2_data7 */ \
	MV(CP(DPM_EMU13),	(M5)) /* dispc2_data6 */ \
	MV(CP(DPM_EMU14),	(M5)) /* dispc2_data5 */ \
	MV(CP(DPM_EMU15),	(M5)) /* dispc2_data4 */ \
	MV(CP(DPM_EMU16),	(M5)) /* dispc2_data3/dmtimer8_pwm_evt */ \
	MV(CP(DPM_EMU17),	(M5)) /* dispc2_data2 */ \
	MV(CP(DPM_EMU18),	(M5)) /* dispc2_data1 */ \
	MV(CP(DPM_EMU19),	(M5)) /* dispc2_data0 */ \
	MV1(WK(PAD0_SIM_IO),	(M0_SAFE)) /* sim_io */ \
	MV1(WK(PAD1_SIM_CLK),	(M0_SAFE)) /* sim_clk */ \
	MV1(WK(PAD0_SIM_RESET),	(M0_SAFE)) /* sim_reset */ \
	MV1(WK(PAD1_SIM_CD),	(M0_SAFE)) /* sim_cd */ \
	MV1(WK(PAD0_SIM_PWRCTRL),	(M0_SAFE)) /* sim_pwrctrl */ \
	MV1(WK(PAD1_SR_SCL),	(PTU | IEN | M0)) /* sr_scl */ \
	MV1(WK(PAD0_SR_SDA),	(PTU | IEN | M0)) /* sr_sda */ \
	MV1(WK(PAD1_FREF_XTAL_IN),	(M0_SAFE)) /* # */ \
	MV1(WK(PAD0_FREF_SLICER_IN),	(M0_SAFE)) /* fref_slicer_in */ \
	MV1(WK(PAD1_FREF_CLK_IOREQ),	(M0_SAFE)) /* fref_clk_ioreq */ \
	MV1(WK(PAD0_FREF_CLK0_OUT),	(M0)) /* sys_drm_msecure */ \
	MV1(WK(PAD1_FREF_CLK3_REQ),	(M0)) /* # */ \
	MV1(WK(PAD0_FREF_CLK3_OUT),	(M0_SAFE)) /* fref_clk3_out */ \
	MV1(WK(PAD1_FREF_CLK4_REQ),	(M0_SAFE)) /* # */ \
	MV1(WK(PAD0_FREF_CLK4_OUT),	(M0_SAFE)) /* # */ \
	MV1(WK(PAD1_SYS_32K),	(IEN | M0_SAFE)) /* sys_32k */ \
	MV1(WK(PAD0_SYS_NRESPWRON),	(IEN | M0_SAFE)) /* sys_nrespwron */ \
	MV1(WK(PAD1_SYS_NRESWARM),	(IEN | M0_SAFE)) /* sys_nreswarm */ \
	MV1(WK(PAD0_SYS_PWR_REQ),	(M0_SAFE)) /* sys_pwr_req */ \
	MV1(WK(PAD1_SYS_PWRON_RESET),	(M3_SAFE)) /* gpio_wk29 */ \
	MV1(WK(PAD0_SYS_BOOT6),	(M3_SAFE)) /* gpio_wk9 */ \
	MV1(WK(PAD1_SYS_BOOT7),	(M3_SAFE)) /* gpio_wk10 */ \
	//MV1(WK(PAD0_JTAG_NTRST),	(IEN | M0)) /* jtag_ntrst */ \
	MV1(WK(PAD1_JTAG_TCK),	(IEN | M0)) /* jtag_tck */ \
	MV1(WK(PAD0_JTAG_RTCK),	(M0)) /* jtag_rtck */ \
	MV1(WK(PAD1_JTAG_TMS_TMSC),	(IEN | M0)) /* jtag_tms_tmsc */ \
	MV1(WK(PAD0_JTAG_TDI),	(IEN | M0)) /* jtag_tdi */ \
	MV1(WK(PAD1_JTAG_TDO),	(M0)) 		  /* jtag_tdo */ 

/**********************************************************
 * Routine: set_muxconf_regs
 * Description: Setting up the configuration Mux registers
 *              specific to the hardware. Many pins need
 *              to be moved from protect to primary mode.
 *********************************************************/
void set_muxconf_regs(void)
{
	MUX_DEFAULT_OMAP4();
	return;
}

/******************************************************************************
 * Routine: update_mux()
 * Description:Update balls which are different between boards.  All should be
 *             updated to match functionality.  However, I'm only updating ones
 *             which I'll be using for now.  When power comes into play they
 *             all need updating.
 *****************************************************************************/
void update_mux(u32 btype, u32 mtype)
{
	/* REVISIT  */
	return;

}

/* optionally do something like blinking LED */
void board_hang (void)
{ while (0) {};}

int nand_init(void)
{
	return 1;
}
void reset_phy(unsigned int base)
{
	*(volatile int*)(base + IODFT_TLGC) |= (1 << 10);
}
