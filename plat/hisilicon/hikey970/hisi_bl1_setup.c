/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <arm_gic.h>
#include <assert.h>
#include <bl_common.h>
#include <console.h>
#include <debug.h>
#include <delay_timer.h>
#include <dw_ufs.h>
#include <errno.h>
#include <generic_delay_timer.h>
#include <gicv2.h>
#include <hisi_regs.h>
#include <mmio.h>
#include <platform.h>
#include <platform_def.h>
#include <string.h>
#include <tbbr/tbbr_img_desc.h>
#include <ufs.h>
#include "../../bl1/bl1_private.h"
#include <hisi_private.h>
#include <hisi_setup_op.h>

enum {
	BOOT_MODE_RECOVERY = 0,
	BOOT_MODE_NORMAL,
	BOOT_MODE_MASK = 1,
};

/*
 * Declarations of linker defined symbols which will help us find the layout
 * of trusted RAM
 */
extern unsigned long __COHERENT_RAM_START__;
extern unsigned long __COHERENT_RAM_END__;

/*
 * The next 2 constants identify the extents of the coherent memory region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __COHERENT_RAM_START__ and __COHERENT_RAM_END__ linker symbols refer to
 * page-aligned addresses.
 */
#define BL1_COHERENT_RAM_BASE (unsigned long)(&__COHERENT_RAM_START__)
#define BL1_COHERENT_RAM_LIMIT (unsigned long)(&__COHERENT_RAM_END__)

/* Data structure which holds the extents of the trusted RAM for BL1 */
static meminfo_t bl1_tzram_layout;

/******************************************************************************
 * On a GICv2 system, the Group 1 secure interrupts are treated as Group 0
 * interrupts.
 *****************************************************************************/
const unsigned int g0_interrupt_array[] = {
	IRQ_SEC_PHY_TIMER,
	IRQ_SEC_SGI_0
};

const gicv2_driver_data_t hisi_gic_data = {
	.gicd_base = GICD_REG_BASE,
	.gicc_base = GICC_REG_BASE,
	.g0_interrupt_num = ARRAY_SIZE(g0_interrupt_array),
	.g0_interrupt_array = g0_interrupt_array,
};

meminfo_t *bl1_plat_sec_mem_layout(void)
{
	return &bl1_tzram_layout;
}

#if LOAD_IMAGE_V2
/*******************************************************************************
 * Function that takes a memory layout into which BL2 has been loaded and
 * populates a new memory layout for BL2 that ensures that BL1's data sections
 * resident in secure RAM are not visible to BL2.
 ******************************************************************************/
void bl1_init_bl2_mem_layout(const meminfo_t *bl1_mem_layout,
			     meminfo_t *bl2_mem_layout)
{

	assert(bl1_mem_layout != NULL);
	assert(bl2_mem_layout != NULL);

	/*
	 * Cannot remove BL1 RW data from the scope of memory visible to BL2
	 * like arm platforms because they overlap in hikey970
	 */
	bl2_mem_layout->total_base = BL2_BASE;
	bl2_mem_layout->total_size = NS_BL1U_LIMIT - BL2_BASE;

	flush_dcache_range((unsigned long)bl2_mem_layout, sizeof(meminfo_t));
}
#endif /* LOAD_IMAGE_V2 */

/*
 * Perform any BL1 specific platform actions.
 */
void bl1_early_platform_setup(void)
{
	unsigned int uart_base;

	generic_delay_timer_init();
	uart_base = PL011_UART6_BASE;
	/* Initialize the console to provide early debug support */
	console_init(uart_base, PL011_UART_CLK_IN_HZ, PL011_BAUDRATE);

	/* Allow BL1 to see the whole Trusted RAM */
	bl1_tzram_layout.total_base = BL1_RW_BASE;
	bl1_tzram_layout.total_size = BL1_RW_SIZE;

#if !LOAD_IMAGE_V2
	/* Calculate how much RAM BL1 is using and how much remains free */
	bl1_tzram_layout.free_base = BL1_RW_BASE;
	bl1_tzram_layout.free_size = BL1_RW_SIZE;
	reserve_mem(&bl1_tzram_layout.free_base,
		    &bl1_tzram_layout.free_size,
		    BL1_RAM_BASE,
		    BL1_RAM_LIMIT - BL1_RAM_BASE); /* bl1_size */
#endif /* LOAD_IMAGE_V2 */

	INFO("BL1: 0x%lx - 0x%lx [size = %lu]\n", BL1_RAM_BASE, BL1_RAM_LIMIT,
	     BL1_RAM_LIMIT - BL1_RAM_BASE); /* bl1_size */
}

/*
 * Perform the very early platform specific architecture setup here. At the
 * moment this only does basic initialization. Later architectural setup
 * (bl1_arch_setup()) does not do anything platform specific.
 */
void bl1_plat_arch_setup(void)
{
	hisi_init_mmu_el3(bl1_tzram_layout.total_base,
			      bl1_tzram_layout.total_size,
			      BL1_RO_BASE,
			      BL1_RO_LIMIT,
			      BL1_COHERENT_RAM_BASE,
			      BL1_COHERENT_RAM_LIMIT);
}

static void hisi_clk_init(void)
{
	unsigned int value;

	/* I2C3/4/7 clk source set 19.2MHz */
	value = 1<<(I2C347_CLK_MUX_SHIFT + 16);
	mmio_write_32(CRG_CLKDIV1_REG, value);
}

static void hisi_ufs_reset(void)
{
	unsigned int reg;

	/* ensure the subsys is disreset */
	mmio_setbits_32(SCTRL_SCPERRSTDIS1_REG, IP_RST_UFS_SUBSYS);

	/* open the subsys's gate clk */
	mmio_write_32(CRG_PEREN0_REG, GT_CLK_NOC_MMC2UFSAPB);

	/*low temp 207m*/
	mmio_write_32(SCTRL_SCPERDIS4_REG, 1 << 14);
	mmio_write_32(SCTRL_SCCLKDIV9_REG, 0x003F0007);
	mmio_write_32(SCTRL_SCPEREN4_REG, 1 << 14);

	/*step 1.HC reset_n enable*/
	mmio_write_32(UFS_SYS_UFS_CRG_UFS_CFG_REG, MASK_IP_RST_UFS);

	/*step 2.SEL RHOLD already config in onchiprom*/
	if (mmio_read_32(SCTRL_DEEPSLEEPED_REG) & EFUSE_RHOLD_BIT) {
		mmio_write_32(UFS_SYS_UFS_DEVICE_RESET_CTRL_REG,
				MASK_UFS_MPHY_RHOLD | BIT_UFS_MPHY_RHOLD);
	}

	/*step 3.HC_PSW powerup*/
	mmio_setbits_32(UFS_SYS_PSW_POWER_CTRL_REG, BIT_UFS_PSW_MTCMOS_EN);
	mdelay(1);
	/*config powerup indicate signal*/
	mmio_setbits_32(UFS_SYS_HC_LP_CTRL_REG, BIT_SYSCTRL_PWR_READY);

	/*step 4.close mphy ref clk*/
	mmio_clrbits_32(UFS_SYS_PHY_CLK_CTRL_REG, BIT_SYSCTRL_REF_CLOCK_EN);

	/* Boston ASIC clk_ufsphy_cfg = sysbus freq /4 div */
	/* Bit[4:2], MASKBITS[2+16:4+16] */
	/* sysbusfreq = 237M, 237/4 = 59.2M */
	reg = (0x3 << 2) | (0x7 << (2 + 16));
	mmio_write_32(UFS_SYS_UFS_CRG_UFS_CFG_REG, reg);

	/*step 5.according curent CG_cfgclk freq set cfg_clock_freq*/
	reg = mmio_read_32(UFS_SYS_PHY_CLK_CTRL_REG);
	reg = (reg & ~MASK_SYSCTRL_CFG_CLOCK_FREQ) | 0x34;
	mmio_write_32(UFS_SYS_PHY_CLK_CTRL_REG, reg);

	/*step 6.set HC ref_clock_sel ti 19.2M*/
	mmio_clrbits_32(UFS_SYS_PHY_CLK_CTRL_REG, MASK_SYSCTRL_REF_CLOCK_SEL);

	/*step 7.enable MPHY inline ref_clk_en*/
	mmio_write_32(UFS_SYS_UFS_DEVICE_RESET_CTRL_REG, 0x00300030);

	/*step 8.bypass ufs clk gate */
	mmio_setbits_32(UFS_SYS_CLOCK_GATE_BYPASS_REG,
			MASK_UFS_CLK_GATE_BYPASS);
	mmio_setbits_32(UFS_SYS_UFS_SYSCTRL_REG, MASK_UFS_SYSCTRL_BYPASS);

	/*step 9.open psw clk*/
	mmio_setbits_32(UFS_SYS_PSW_CLK_CTRL_REG, BIT_SYSCTRL_PSW_CLK_EN);

	/*step 10.relieve UFSHC PSW iso enbaled*/
	mmio_clrbits_32(UFS_SYS_PSW_POWER_CTRL_REG, BIT_UFS_PSW_ISO_CTRL);
	/* phy iso only effective on Miami,
	 * double check for kirin980 and later plat
	 */
	/* disable phy iso */
	mmio_clrbits_32(UFS_SYS_PHY_ISO_EN_REG, BIT_UFS_PHY_ISO_CTRL);
	/* notice iso disable */
	mmio_clrbits_32(UFS_SYS_HC_LP_CTRL_REG, BIT_SYSCTRL_LP_ISOL_EN);

	/*step 11.HC aresetn and disable lp_reset*/
	mmio_write_32(UFS_SYS_UFS_CRG_UFS_CFG_REG,
		      MASK_IP_ARST_UFS | BIT_IP_ARST_UFS);
	mmio_setbits_32(UFS_SYS_RESET_CTRL_EN_REG, BIT_SYSCTRL_LP_RESET_N);

	/*step 12.open mphy ref clk*/
	mmio_setbits_32(UFS_SYS_PHY_CLK_CTRL_REG, BIT_SYSCTRL_REF_CLOCK_EN);

	/***Device Clock reset config***/
	/*step 1.en Device reset pad valid*/
	mmio_write_32(UFS_SYS_UFS_DEVICE_RESET_CTRL_REG, MASK_UFS_DEVICE_RESET);
	/*step 2.open ufs device ref clock*/
	mmio_write_32(SCTRL_SCPEREN4_REG, GT_CLK_UFSIO_REF);
	/*step 3.*/
	mdelay(2);
	/*step 4.en Device reset pad invalid*/
	mmio_write_32(UFS_SYS_UFS_DEVICE_RESET_CTRL_REG,
		      MASK_UFS_DEVICE_RESET | BIT_UFS_DEVICE_RESET);
	/*step 5.wait 10ms*/
	mdelay(40);
	/* enable the fix of linereset recovery,
	 * enable rx_reset/tx_rest beat
	 * enable ref_clk_en override(bit5) & override value = 1(bit4) with mask
	 */
	mmio_write_32(UFS_SYS_UFS_CRG_UFS_CFG_REG,
		      MASK_IP_RST_UFS | BIT_IP_RST_UFS);

	mdelay(1);
}

static void hisi_ufs_init(void)
{
	dw_ufs_params_t ufs_params;

	memset(&ufs_params, 0, sizeof(ufs_params));
	ufs_params.reg_base = UFS_REG_BASE;
	ufs_params.desc_base = HISI_UFS_DESC_BASE;
	ufs_params.desc_size = HISI_UFS_DESC_SIZE;
	ufs_params.flags = UFS_FLAGS_PHY_10nm;

	if ((ufs_params.flags & UFS_FLAGS_SKIPINIT) == 0)
		hisi_ufs_reset();
	dw_ufs_init(&ufs_params);
}

static void hisi_peri_init(void)
{
	/* unreset */
	mmio_setbits_32(CRG_PERRSTDIS4_REG, 1);
}

static void hisi_pinmux_init(void)
{
	mmio_write_32(IOMG_AO_033_REG, 1);

}

/*
 * Function which will perform any remaining platform-specific setup that can
 * occur after the MMU and data cache have been enabled.
 */
void bl1_platform_setup(void)
{
	hisi_clk_init();
	hisi_regulator_enable();
	hisi_peri_init();
	hisi_ufs_init();
	hisi_pinmux_init();
	hisi_io_setup();
}

/*
 * The following function checks if Firmware update is needed,
 * by checking if TOC in FIP image is valid or not.
 */
unsigned int bl1_plat_get_next_image_id(void)
{
	unsigned int mode, ret;

	mode = mmio_read_32(SCTRL_BAK_DATA0_REG);
	switch (mode & BOOT_MODE_MASK) {
	case BOOT_MODE_RECOVERY:
		ret = NS_BL1U_IMAGE_ID;
		break;
	case BOOT_MODE_NORMAL:
		ret = BL2_IMAGE_ID;
		break;
	default:
		WARN("Invalid boot mode is found:%d\n", mode);
		panic();
	}
	return ret;
}

image_desc_t *bl1_plat_get_image_desc(unsigned int image_id)
{
	unsigned int index = 0;

	while (bl1_tbbr_image_descs[index].image_id != INVALID_IMAGE_ID) {
		if (bl1_tbbr_image_descs[index].image_id == image_id)
			return &bl1_tbbr_image_descs[index];
		index++;
	}

	return NULL;
}

void bl1_plat_set_ep_info(unsigned int image_id,
		entry_point_info_t *ep_info)
{
	unsigned int data = 0;
	uintptr_t tmp = HISI_NS_TMP_OFFSET;

	if (image_id == BL2_IMAGE_ID)
		return;
	/* Copy NS BL1U from 0x1681_8000 to 0x1689_8000 */
	memcpy((void *)tmp, (void *)HISI_NS_IMAGE_OFFSET,
		NS_BL1U_SIZE);
	memcpy((void *)NS_BL1U_BASE, (void *)tmp, NS_BL1U_SIZE);
	inv_dcache_range(NS_BL1U_BASE, NS_BL1U_SIZE);
	/* Initialize the GIC driver, cpu and distributor interfaces */
	gicv2_driver_init(&hisi_gic_data);
	gicv2_distif_init();
	gicv2_pcpu_distif_init();
	gicv2_cpuif_enable();
	/* CNTFRQ is read-only in EL1 */
	write_cntfrq_el0(plat_get_syscnt_freq2());
	data = read_cpacr_el1();
	do {
		data |= 3 << 20;
		write_cpacr_el1(data);
		data = read_cpacr_el1();
	} while ((data & (3 << 20)) != (3 << 20));
	INFO("cpacr_el1:0x%x\n", data);

	ep_info->args.arg0 = 0xffff & read_mpidr();
	ep_info->spsr = SPSR_64(MODE_EL1, MODE_SP_ELX,
				DISABLE_ALL_EXCEPTIONS);
}
