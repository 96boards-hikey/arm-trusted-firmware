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
#include <errno.h>
#include <generic_delay_timer.h>
#include <gicv2.h>
#include <hisi_regs.h>
#include <mmio.h>
#include <platform.h>
#include <platform_def.h>
#include <string.h>
#include <hisi_private.h>
#include <hisi_setup_op.h>
#include <hisi_service.h>

#ifndef BIT
#define BIT(n)	(1 << (n))
#define BITMSK(x)                   (BIT(x) << 16)
#endif
#define REVERSE_BIT(n)	(~(1 << (n)))

/***********************NOC BEGIN******************/
#define SOC_PMCTRL_NOC_POWER_IDLEREQ_VIVOBUS	(1 << 15)
#define SOC_PMCTRL_NOC_POWER_IDLEREQ_IVP	(1 << 14)
#define SOC_PMCTRL_NOC_POWER_IDLEREQ_DSS	(1 << 13)
#define SOC_PMCTRL_NOC_POWER_IDLEREQ_VENC	(1 << 11)
#define SOC_PMCTRL_NOC_POWER_IDLEREQ_VDEC	(1 << 10)
#define SOC_PMCTRL_NOC_POWER_IDLEREQ_ICS	(1 << 9)
#define SOC_PMCTRL_NOC_POWER_IDLEREQ_ISP	(1 << 5)
#define SOC_PMCTRL_NOC_POWER_IDLEREQ_VCODEC     (1 << 4)
#define SET_SOC_PMCTRL_NOC_IDLE_MOD_MASK		16
#define SOC_MODULE_NOC_TIMEOUT			100
/***********************NOC END******************/

enum ip_regulator_id {

	IP_REGULATOR_MEDIA1_SUBSYS_ID = 0,
	IP_REGULATOR_VIVOBUS_ID,
	IP_REGULATOR_VCODEC_ID,
	IP_REGULATOR_DSS_ID,
	IP_REGULATOR_ISP_ID,
	IP_REGULATOR_VDEC_ID,
	IP_REGULATOR_VENC_ID,
	IP_REGULATOR_ISP_SRT_ID,
	IP_REGULATOR_MEDIA2_SUBSYS_ID,
	IP_REGULATOR_ID_MAX,
};

static void autofs_mediacrg1_init_set(void)
{
	/*vivobus autofs init*/
	mmio_write_32(MEDIA1_PERI_AUTODIV4_REG, 0x07E0283F);
	/*ivp autofs init*/
	mmio_write_32(MEDIA1_PERI_AUTODIV0_REG, 0x07E0283F);
	/*isp autofs init*/
	mmio_write_32(MEDIA1_PERI_AUTODIV1_REG, 0x07E0283F);
}

static void bus_idle_clear(unsigned int value)
{
	unsigned int pmc_value, pmc_value1, pmc_value2;
	int time_count = SOC_MODULE_NOC_TIMEOUT;

	pmc_value = value << SET_SOC_PMCTRL_NOC_IDLE_MOD_MASK;
	pmc_value &=  (~value);
	mmio_write_32(PMC_NOC_POWER_IDLEREQ_REG, pmc_value);

	for (;;) {
		pmc_value1 =
			(unsigned int)mmio_read_32(PMC_NOC_POWER_IDLEACK_REG);
		pmc_value2 = (unsigned int)mmio_read_32(PMC_NOC_POWER_IDLE_REG);
		if (((pmc_value1 & value) == 0) && ((pmc_value2 & value) == 0))
			break;
		udelay(1);
		time_count--;
		if (time_count <= 0) {
			WARN("%s timeout\n", __func__);
			break;
		}
	}
}

static void bus_idle_set(unsigned int value)
{
	unsigned int  pmc_value, pmc_value1, pmc_value2;
	int time_count = SOC_MODULE_NOC_TIMEOUT;

	pmc_value = value | (value << SET_SOC_PMCTRL_NOC_IDLE_MOD_MASK);
	mmio_write_32(PMC_NOC_POWER_IDLEREQ_REG, pmc_value);

	for (;;) {
		pmc_value1 =
			(unsigned int)mmio_read_32(PMC_NOC_POWER_IDLEACK_REG);
		pmc_value2 = (unsigned int)mmio_read_32(PMC_NOC_POWER_IDLE_REG);
		if (((pmc_value1 & value) != 0) && ((pmc_value2 & value) != 0))
			break;
		udelay(1);
		time_count--;
		if (time_count <= 0) {
			WARN("%s timeout\n", __func__);
			break;
		}
	}
}

static void set_media1_subsys_power_up(void)
{
	/*1:module mtcmos on*/
	mmio_write_32(CRG_PERPWREN_REG, 0x00000020);
	udelay(100);

	/*1.1:moduel unrst*/
	mmio_write_32(CRG_PERRSTDIS5_REG, 0x00040000);

	/*2:module clk enable*/
	mmio_write_32(CRG_PEREN6_REG, 0x7C002028);
	mmio_write_32(SCTRL_SCPEREN4_REG, 0x00000040);
	mmio_write_32(CRG_PEREN4_REG, 0x00000040);
	udelay(1);

	/*3:module clk disable*/
	mmio_write_32(CRG_PERDIS6_REG, 0x7C002028);
	mmio_write_32(SCTRL_SCPERDIS4_REG, 0x00000040);
	mmio_write_32(CRG_PERDIS4_REG, 0x00000040);
	udelay(1);

	/*4:module iso disable*/
	mmio_write_32(CRG_ISODIS_REG, 0x00000040);

	/*5:memory rempair*/
	//hisi_ip_mem_repair(MRB_DSS);
	/*6:module unrst*/
	mmio_write_32(CRG_PERRSTDIS5_REG, 0x00020000);

	/*7:module clk enable*/
	mmio_write_32(CRG_PEREN6_REG, 0x7C002028);
	mmio_write_32(SCTRL_SCPEREN4_REG, 0x00000040);
	mmio_write_32(CRG_PEREN4_REG, 0x00000040);

	/*8:bus idle clear*/

	/*9: set autofs int*/
	autofs_mediacrg1_init_set();

}

static void set_media1_subsys_power_down(void)
{
	/*1:bus idle set*/

	/*2:module rst*/
	mmio_write_32(CRG_PERRSTEN5_REG, 0x00020000);

	/*3:module clk disable*/
	mmio_write_32(CRG_PERDIS6_REG, 0x7C002028);
	mmio_write_32(SCTRL_SCPERDIS4_REG, 0x00000040);
	mmio_write_32(CRG_PERDIS4_REG, 0x00000040);

	/*4:module iso*/
	mmio_write_32(CRG_ISOEN_REG, 0x00000040);

	/*4.2 module unrst*/
	mmio_write_32(CRG_PERRSTEN5_REG, 0x00040000);

	/*5:module mtcmos off*/
	mmio_write_32(CRG_PERPWRDIS_REG, 0x00000020);

}

static void set_vivobus_power_up(void)
{
	/* vivobus needs freq smaller than 0.65v freq */
	mmio_write_32(MEDIA1_CRG_CLKDIV5_REG, 0x003F0005);

	/*1:module clk enable*/
	mmio_write_32(MEDIA1_CRG_CLKDIV9_REG, 0x00080008);
	mmio_write_32(MEDIA1_CRG_PEREN0_REG, 0x08040040);
	udelay(1);

	/*2:module clk disable*/
	mmio_write_32(MEDIA1_CRG_PERDIS0_REG, 0x00040040);
	udelay(1);

	/*3:module unrst*/

	/*4:module clk enable*/
	mmio_write_32(MEDIA1_CRG_PEREN0_REG, 0x00040040);

	/*5:bus idle clear*/
	bus_idle_clear(SOC_PMCTRL_NOC_POWER_IDLEREQ_VIVOBUS);

}

static void set_vivobus_power_down(void)
{
	/*1:bus idle set*/
	bus_idle_set(SOC_PMCTRL_NOC_POWER_IDLEREQ_VIVOBUS);

	/*3:module clk disable*/
	mmio_write_32(MEDIA1_CRG_PERDIS0_REG, 0x08000000);
	mmio_write_32(MEDIA1_CRG_PERDIS0_REG, 0x00040040);
	mmio_write_32(MEDIA1_CRG_CLKDIV9_REG, 0x00080000);

}

static void set_dss_power_up(void)
{
	/*open ppll0_media*/
	mmio_write_32(SCTRL_SCPEREN4_REG, 0x00000040);

	/*set ldi0 CLOCK, choose ppll0 , 17div ,97.6M*/
	mmio_write_32(MEDIA1_CRG_CLKDIV0_REG, 0x003F0010);

	/*set edc0 CLOCK*/
	mmio_write_32(MEDIA1_CRG_CLKDIV2_REG, 0x003F0010);

	/*1:module mtcmos on*/

	/*1.1 DISP CRG*/
	mmio_write_32(MEDIA1_CRG_PERRSTDIS0_REG, 0x02000000);

	/*2:module clk enable*/
	mmio_write_32(MEDIA1_CRG_CLKDIV9_REG, 0xCA80CA80);
	mmio_write_32(MEDIA1_CRG_PEREN0_REG, 0x0009C000);
	mmio_write_32(MEDIA1_CRG_PEREN1_REG, 0x00660000);
	mmio_write_32(MEDIA1_CRG_PEREN2_REG, 0x0000003F);
	udelay(1);

	/*3:module clk disable*/
	mmio_write_32(MEDIA1_CRG_PERDIS0_REG, 0x0009C000);
	mmio_write_32(MEDIA1_CRG_PERDIS1_REG, 0x00600000);
	mmio_write_32(MEDIA1_CRG_PERDIS2_REG, 0x0000003F);
	udelay(1);

	/*4:module iso disable*/

	/*5:memory rempair*/

	/*6:module unrst*/
	mmio_write_32(MEDIA1_CRG_PERRSTDIS0_REG, 0x000000C0);
	mmio_write_32(MEDIA1_CRG_PERRSTDIS1_REG, 0x000000F0);

	/*7:module clk enable*/
	mmio_write_32(MEDIA1_CRG_PEREN0_REG, 0x0009C000);
	mmio_write_32(MEDIA1_CRG_PEREN1_REG, 0x00600000);
	mmio_write_32(MEDIA1_CRG_PEREN2_REG, 0x0000003F);

	/*8:bus idle clear*/
	bus_idle_clear(SOC_PMCTRL_NOC_POWER_IDLEREQ_DSS);

}

static void set_dss_power_down(void)
{
	/*1:bus idle set*/
	bus_idle_set(SOC_PMCTRL_NOC_POWER_IDLEREQ_DSS);

	/*2:module rst*/
	mmio_write_32(MEDIA1_CRG_PERRSTEN0_REG, 0x001C00C0);
	mmio_write_32(MEDIA1_CRG_PERRSTEN1_REG, 0x000000F0);

	/*3:module clk disable*/
	mmio_write_32(MEDIA1_CRG_PERDIS0_REG, 0x0009C000);
	mmio_write_32(MEDIA1_CRG_PERDIS1_REG, 0x00600000);
	mmio_write_32(MEDIA1_CRG_PERDIS2_REG, 0x0000003F);
	mmio_write_32(MEDIA1_CRG_PERDIS1_REG, 0x00060000);
	mmio_write_32(MEDIA1_CRG_CLKDIV9_REG, 0xCA80CA80);

	/*4:module iso*/

	/*4.1: crg rst*/
	mmio_write_32(MEDIA1_CRG_PERRSTEN0_REG, 0x02000000);

}

static void set_vcodec_power_up(void)
{
	/*1:module clk enable*/
	mmio_write_32(CRG_CLKDIV18_REG, 0x01000100);
	mmio_write_32(CRG_PEREN0_REG, 0x00000020);
	mmio_write_32(MEDIA2_CRG_PEREN0_REG, 0x00000200);
	udelay(1);

	/*2:module clk disable*/
	mmio_write_32(MEDIA2_CRG_PERDIS0_REG, 0x00000200);
	mmio_write_32(CRG_PERDIS0_REG, 0x00000020);
	udelay(1);

	/*3:module unrst*/

	/*4:module clk enable*/
	mmio_write_32(CRG_PEREN0_REG, 0x00000020);
	mmio_write_32(MEDIA2_CRG_PEREN0_REG, 0x00000200);

	/*3:bus idle clear*/
	bus_idle_clear(SOC_PMCTRL_NOC_POWER_IDLEREQ_VCODEC);

}

static void set_vcodec_power_down(void)
{
	/*1:bus idle set*/
	bus_idle_set(SOC_PMCTRL_NOC_POWER_IDLEREQ_VCODEC);

	/*2:module rst*/

	/*3:module clk disable*/
	mmio_write_32(MEDIA2_CRG_PERDIS0_REG, 0x00000200);
	mmio_write_32(CRG_PERDIS0_REG, 0x00000020);
	mmio_write_32(CRG_CLKDIV18_REG, 0x01000000);

}

static void set_vdec_power_up(void)
{
	/*1:module mtcmos on*/
	mmio_write_32(CRG_PERPWREN_REG, 0x00000004);
	udelay(100);

	/*2:module clk enable*/
	mmio_write_32(CRG_CLKDIV18_REG, 0x20002000);
	mmio_write_32(MEDIA2_CRG_PEREN0_REG, 0x000001C0);
	udelay(1);

	/*3:module clk disable*/
	mmio_write_32(MEDIA2_CRG_PERDIS0_REG, 0x000001C0);
	udelay(1);

	/*4:module iso disable*/
	mmio_write_32(CRG_ISODIS_REG, 0x00000004);

	/*5:memory rempair*/
	//hisi_ip_mem_repair(MRB_VDEC_SUBSYS);

	/*6:module unrst*/
	mmio_write_32(MEDIA2_CRG_PERRSTDIS0_REG, 0x00000040);

	/*7:module clk enable*/
	mmio_write_32(MEDIA2_CRG_PEREN0_REG, 0x000001C0);

	/*8:bus idle clear*/
	bus_idle_clear(SOC_PMCTRL_NOC_POWER_IDLEREQ_VDEC);

	/*9:2p memory init*/
	mmio_write_32(VDEC_CFG_BASE + 0xC074, 0x03400260);
	mmio_write_32(VDEC_CFG_BASE + 0xF008, 0x02605550);

}

static void set_vdec_power_down(void)
{
	/*1:bus idle set*/
	bus_idle_set(SOC_PMCTRL_NOC_POWER_IDLEREQ_VDEC);

	/*2:module rst*/
	mmio_write_32(MEDIA2_CRG_PERRSTEN0_REG, 0x00000040);

	/*3:module clk disable*/
	mmio_write_32(MEDIA2_CRG_PERDIS0_REG, 0x000001C0);
	mmio_write_32(CRG_CLKDIV18_REG, 0x20000000);

	/*4:module iso*/
	mmio_write_32(CRG_ISOEN_REG, 0x00000004);

	/*5:module mtcmos off*/
	mmio_write_32(CRG_PERPWRDIS_REG, 0x00000004);

}

static void set_venc_power_up(void)
{
	/*open ppll0_media*/
	mmio_write_32(SCTRL_SCPEREN4_REG, 0x00000040);
	/*set VENC CLOCK, div 18 ,1660/18 = 92.2M*/
	mmio_write_32(CRG_CLKDIV6_REG, 0x3F001100);

	/*1:module mtcmos on*/
	mmio_write_32(CRG_PERPWREN_REG, 0x00000002);
	udelay(100);

	/*2:module clk enable*/
	mmio_write_32(CRG_CLKDIV18_REG, 0x02000200);
	mmio_write_32(MEDIA2_CRG_PEREN0_REG, 0x00000038);
	udelay(1);

	/*3:module clk disable*/
	mmio_write_32(MEDIA2_CRG_PERDIS0_REG, 0x00000038);
	udelay(1);

	/*4:module iso disable*/
	mmio_write_32(CRG_ISODIS_REG, 0x00000002);

	/*5:memory rempair*/
	//hisi_ip_mem_repair(MRB_VENC_SUBSYS);

	/*6:module unrst*/
	mmio_write_32(MEDIA2_CRG_PERRSTDIS0_REG, 0x00000007);

	/*7:module clk enable*/
	mmio_write_32(MEDIA2_CRG_PEREN0_REG, 0x00000038);

	/*8:bus idle clear*/
	bus_idle_clear(SOC_PMCTRL_NOC_POWER_IDLEREQ_VENC);

	/*9:2p memory init*/
	mmio_write_32(VENC_CFG_BASE + 0x1B0, 0x02605550);
	mmio_write_32(VENC_CFG_BASE + 0x10008, 0x02605550);

}

static void set_venc_power_down(void)
{
	/*1:bus idle set*/
	bus_idle_set(SOC_PMCTRL_NOC_POWER_IDLEREQ_VENC);

	/*2:module rst*/
	mmio_write_32(MEDIA2_CRG_PERRSTEN0_REG, 0x00000007);

	/*3:module clk disable*/
	mmio_write_32(MEDIA2_CRG_PERDIS0_REG, 0x00000038);
	mmio_write_32(CRG_CLKDIV18_REG, 0x02000000);

	/*4:module iso*/
	mmio_write_32(CRG_ISOEN_REG, 0x00000002);

	/*5:module mtcmos off*/
	mmio_write_32(CRG_PERPWRDIS_REG, 0x00000002);

}

static void set_isp_power_up(void)
{
	/*1:module mtcmos on*/
	mmio_write_32(CRG_PERPWREN_REG, 0x00000001);
	udelay(100);

	/*1.1:module unrst*/
	mmio_write_32(MEDIA1_CRG_PERRSTDIS0_REG, 0x04000000);

	/*2:module clk enable*/
	mmio_write_32(MEDIA1_CRG_CLKDIV9_REG, 0x30003000);
	mmio_write_32(MEDIA1_CRG_PEREN1_REG, 0x1E198000);
	udelay(1);

	/*3:module clk disable*/
	mmio_write_32(MEDIA1_CRG_PERDIS1_REG, 0x1E018000);
	udelay(1);

	/*4:module iso disable*/
	mmio_write_32(CRG_ISODIS_REG, 0x00000001);

	/*5:memory rempair*/
	//hisi_ip_mem_repair(MRB_ISP_SUBSYS_TOP);

	/*6:module unrst*/
	mmio_write_32(MEDIA1_CRG_PERRSTDIS0_REG, 0x01E00000);
	mmio_write_32(MEDIA1_CRG_PERRSTDIS1_REG, 0x0000000C);

	/*7:module clk enable*/
	mmio_write_32(MEDIA1_CRG_PEREN1_REG, 0x1E018000);

	/*8:bus idle clear*/
	bus_idle_clear(SOC_PMCTRL_NOC_POWER_IDLEREQ_ISP);

	/*9:csi clk enable*/
}

static void set_isp_power_down(void)
{
	/*1:bus idle set*/
	bus_idle_set(SOC_PMCTRL_NOC_POWER_IDLEREQ_ISP);
	/*2:module rst*/
	mmio_write_32(MEDIA1_CRG_PERRSTEN0_REG, 0x01E04000);
	mmio_write_32(MEDIA1_CRG_PERRSTEN1_REG, 0x0000000C);

	/*3:module clk disable*/
	mmio_write_32(MEDIA1_CRG_PERDIS1_REG, 0x1E018000);
	mmio_write_32(MEDIA1_CRG_PERDIS1_REG, 0x00180000);
	mmio_write_32(MEDIA1_CRG_CLKDIV9_REG, 0x30003000);

	/*4:module iso*/
	mmio_write_32(CRG_ISOEN_REG, 0x00000001);

	/*4.2:module unrst*/
	mmio_write_32(MEDIA1_CRG_PERRSTEN0_REG, 0x04000000);

	/*5:module mtcmos off*/
	mmio_write_32(CRG_PERPWRDIS_REG, 0x00000001);

}

static void set_isp_srt_power_up(void)
{
	/*1:module mtcmos on*/
	mmio_write_32(CRG_PERPWREN_REG, 0x00400000);
	udelay(100);

	/*2:module clk enable*/
	mmio_write_32(CRG_CLKDIV18_REG, 0x10001000);
	mmio_write_32(CRG_CLKDIV20_REG, 0x00100010);
	mmio_write_32(MEDIA1_CRG_CLKDIV9_REG, 0x00600060);
	mmio_write_32(CRG_PEREN3_REG, 0x04000000);
	mmio_write_32(MEDIA1_CRG_PEREN0_REG, 0x40103A00);
	udelay(1);

	/*3:module clk disable*/
	mmio_write_32(CRG_PERDIS3_REG, 0x04000000);
	mmio_write_32(MEDIA1_CRG_PERDIS0_REG, 0x40103A00);
	udelay(1);

	/*4:module iso disable*/
	mmio_write_32(CRG_ISODIS_REG, 0x08000000);

	/*5:memory rempair*/
	//hisi_ip_mem_repair(MRB_ISP_SUBSYS);

	/*6:module unrst*/
	mmio_write_32(MEDIA1_CRG_PERRSTDIS_ISP_SEC_REG, 0x0000006B);

	/*7:module clk enable*/
	mmio_write_32(CRG_PEREN3_REG, 0x04000000);
	mmio_write_32(MEDIA1_CRG_PEREN0_REG, 0x40103A00);

	/*8:module clk enable**/
	mmio_write_32(CRG_PEREN3_REG, 0x00700000);

}

static void set_isp_srt_power_down(void)
{
	/*1:bus idle set*/

	/*2:module rst*/
	mmio_write_32(MEDIA1_CRG_PERRSTEN_ISP_SEC_REG, 0x0000007B);

	/*3:module clk disable*/
	mmio_write_32(CRG_PERDIS3_REG, 0x04000000);
	mmio_write_32(CRG_PERDIS3_REG, 0x00700000);
	mmio_write_32(MEDIA1_CRG_PERDIS0_REG, 0x40103A00);
	mmio_write_32(MEDIA1_CRG_CLKDIV9_REG, 0x00600000);
	mmio_write_32(CRG_CLKDIV20_REG, 0x00100000);
	mmio_write_32(CRG_CLKDIV18_REG, 0x10000000);

	/*4:module iso*/
	mmio_write_32(CRG_ISOEN_REG, 0x08000000);

	/*5:module mtcmos off*/
	mmio_write_32(CRG_PERPWRDIS_REG, 0x00400000);

}

static void set_media2_subsys_power_up(void)
{
	/*1:module mtcmos on*/

	/*1.1:moduel unrst*/
	mmio_write_32(CRG_PERRSTDIS4_REG, 0x00000002);

	/*2:module clk enable*/
	mmio_write_32(CRG_CLKDIV18_REG, 0x01000100);
	mmio_write_32(CRG_PEREN6_REG, 0x00010200);
	mmio_write_32(CRG_PEREN0_REG, 0x00000020);
	udelay(1);

	/*3:module clk disable*/
	mmio_write_32(CRG_PERDIS6_REG, 0x00010200);
	mmio_write_32(CRG_PERDIS0_REG, 0x00000020);
	udelay(1);

	/*4:module iso disable*/

	/*5:memory rempair*/

	/*6:module unrst*/
	mmio_write_32(CRG_PERRSTDIS4_REG, 0x00000001);

	/*7:module clk enable*/
	mmio_write_32(CRG_PEREN6_REG, 0x00010200);

	/*8:bus idle clear*/

}

static void set_media2_subsys_power_down(void)
{
	/*1:bus idle set*/

	/*2:module rst*/
	mmio_write_32(CRG_PERRSTEN4_REG, 0x00000001);

	/*3:module clk disable*/
	mmio_write_32(CRG_PERDIS6_REG, 0x00010200);

	/*4:module iso*/

	/*5:module unrst*/
	mmio_write_32(CRG_PERRSTEN4_REG, 0x00000002);
}

void hisi_regulator_enable(void)
{
	set_media1_subsys_power_up();
	set_media2_subsys_power_up();
	set_vivobus_power_up();
	set_dss_power_up();
	set_vcodec_power_up();
	set_vdec_power_up();
	set_venc_power_up();
	set_isp_power_up();
	set_isp_srt_power_up();
	/* isp module unrst */
	mmio_write_32(0xe8583800, 0x7);
	mmio_write_32(0xe8583804, 0xf);
}

void hisi_regulator_disable(void)
{
	set_isp_srt_power_down();
	set_isp_power_down();
	set_venc_power_down();
	set_vdec_power_down();
	set_vcodec_power_down();
	set_dss_power_down();
	set_vivobus_power_down();
	set_media2_subsys_power_down();
	set_media1_subsys_power_down();
}

static uint64_t hisi_set_regulator_power_up(uint64_t dev_id)
{
	uint64_t ret = 0;

	switch (dev_id) {

	case IP_REGULATOR_MEDIA1_SUBSYS_ID:
		set_media1_subsys_power_up();
		break;
	case IP_REGULATOR_VIVOBUS_ID:
		set_vivobus_power_up();
		break;
	case IP_REGULATOR_VCODEC_ID:
		set_vcodec_power_up();
		break;
	case IP_REGULATOR_DSS_ID:
		set_dss_power_up();
		break;
	case IP_REGULATOR_ISP_ID:
		set_isp_power_up();
		break;
	case IP_REGULATOR_VDEC_ID:
		set_vdec_power_up();
		break;
	case IP_REGULATOR_VENC_ID:
		set_venc_power_up();
		break;
	case IP_REGULATOR_ISP_SRT_ID:
		set_isp_srt_power_up();
		break;
	case IP_REGULATOR_MEDIA2_SUBSYS_ID:
		set_media2_subsys_power_up();
		break;
	default:
		ret = (uint64_t)REGULATOR_E_ERROR;
	}
	return ret;
}

static uint64_t hisi_set_regulator_power_down(uint64_t dev_id)
{
	uint64_t ret = 0;

	switch (dev_id) {

	case IP_REGULATOR_MEDIA1_SUBSYS_ID:
		set_media1_subsys_power_down();
		break;
	case IP_REGULATOR_VIVOBUS_ID:
		set_vivobus_power_down();
		break;
	case IP_REGULATOR_VCODEC_ID:
		set_vcodec_power_down();
		break;
	case IP_REGULATOR_DSS_ID:
		set_dss_power_down();
		break;
	case IP_REGULATOR_ISP_ID:
		set_isp_power_down();
		break;
	case IP_REGULATOR_VDEC_ID:
		set_vdec_power_down();
		break;
	case IP_REGULATOR_VENC_ID:
		set_venc_power_down();
		break;
	case IP_REGULATOR_ISP_SRT_ID:
		set_isp_srt_power_down();
		break;
	case IP_REGULATOR_MEDIA2_SUBSYS_ID:
		set_media2_subsys_power_down();
		break;
	default:
		ret = (uint64_t)REGULATOR_E_ERROR;
	}
	return ret;
}

static const plat_regulator_ops_t hisi_regulator_ops = {
	hisi_set_regulator_power_up,
	hisi_set_regulator_power_down,
};

int platform_regulator_ops_register(const struct plat_regulator_ops **plat_ops)
{
	*plat_ops = &hisi_regulator_ops;
	return REGULATOR_E_SUCCESS;
}
