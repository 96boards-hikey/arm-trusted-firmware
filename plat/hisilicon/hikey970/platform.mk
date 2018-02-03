#
# Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Enable version2 of image loading
LOAD_IMAGE_V2	:=	1

# On Hikey970, the TSP can execute from TZC secure area in DRAM.
HISI_TSP_RAM_LOCATION	:=	dram
ifeq (${HISI_TSP_RAM_LOCATION}, dram)
  HISI_TSP_RAM_LOCATION_ID = HISI_DRAM_ID
else ifeq (${HISI_TSP_RAM_LOCATION}, sram)
  HISI_TSP_RAM_LOCATION_ID := HISI_SRAM_ID
else
  $(error "Currently unsupported HISI_TSP_RAM_LOCATION value")
endif

CRASH_CONSOLE_BASE		:=	PL011_UART6_BASE
COLD_BOOT_SINGLE_CPU		:=	1
PROGRAMMABLE_RESET_ADDRESS	:=	1

# Process flags
$(eval $(call add_define,HISI_TSP_RAM_LOCATION_ID))
$(eval $(call add_define,CRASH_CONSOLE_BASE))

# Add the build options to pack Trusted OS Extra1 and Trusted OS Extra2 images
# in the FIP if the platform requires.
ifneq ($(BL32_EXTRA1),)
$(eval $(call FIP_ADD_IMG,BL32_EXTRA1,--tos-fw-extra1))
endif
ifneq ($(BL32_EXTRA2),)
$(eval $(call FIP_ADD_IMG,BL32_EXTRA2,--tos-fw-extra2))
endif

ENABLE_PLAT_COMPAT	:=	0

USE_COHERENT_MEM	:=	1

PLAT_INCLUDES		:=	-Iinclude/common/tbbr			\
				-Iplat/hisilicon/hikey970/include

PLAT_BL_COMMON_SOURCES	:=	drivers/arm/pl011/pl011_console.S	\
				drivers/delay_timer/delay_timer.c	\
				drivers/delay_timer/generic_delay_timer.c \
				lib/aarch64/xlat_tables.c		\
				plat/hisilicon/hikey970/aarch64/hisi_common.c \
				plat/hisilicon/hikey970/hisi_setup_op.c

HISI_GIC_SOURCES	:=	drivers/arm/gic/common/gic_common.c	\
				drivers/arm/gic/v2/gicv2_main.c		\
				drivers/arm/gic/v2/gicv2_helpers.c	\
				plat/common/plat_gicv2.c

BL1_SOURCES		+=	bl1/tbbr/tbbr_img_desc.c		\
				drivers/io/io_block.c			\
				drivers/io/io_fip.c			\
				drivers/io/io_storage.c			\
				drivers/synopsys/ufs/dw_ufs.c		\
				drivers/ufs/ufs.c 			\
				lib/cpus/aarch64/cortex_a53.S		\
				plat/hisilicon/hikey970/aarch64/hisi_helpers.S \
				plat/hisilicon/hikey970/hisi_bl1_setup.c 	\
				plat/hisilicon/hikey970/hisi_io_storage.c \
				${HISI_GIC_SOURCES}

BL2_SOURCES		+=	drivers/io/io_block.c			\
				drivers/io/io_fip.c			\
				drivers/io/io_storage.c			\
				drivers/ufs/ufs.c			\
				plat/hisilicon/hikey970/hisi_bl2_setup.c \
				plat/hisilicon/hikey970/hisi_io_storage.c \
				plat/hisilicon/hikey970/hisi_mcu_load.c

ifeq (${LOAD_IMAGE_V2},1)
BL2_SOURCES		+=	plat/hisilicon/hikey970/hisi_bl2_mem_params_desc.c \
				plat/hisilicon/hikey970/hisi_image_load.c \
				common/desc_image_load.c

ifeq (${SPD},opteed)
BL2_SOURCES		+=	lib/optee/optee_utils.c
endif
endif

BL31_SOURCES		+=	drivers/arm/cci/cci.c			\
				lib/cpus/aarch64/cortex_a53.S           \
				lib/cpus/aarch64/cortex_a72.S		\
				lib/cpus/aarch64/cortex_a73.S		\
				plat/common/aarch64/plat_psci_common.c  \
				plat/hisilicon/hikey970/aarch64/hisi_helpers.S \
				plat/hisilicon/hikey970/hisi_bl31_setup.c \
				plat/hisilicon/hikey970/hisi_pm.c	\
				plat/hisilicon/hikey970/hisi_topology.c \
				plat/hisilicon/hikey970/drivers/pwrc/hisi_pwrc.c \
				plat/hisilicon/hikey970/drivers/ipc/hisi_ipc.c \
				${HISI_GIC_SOURCES}

# Enable workarounds for selected Cortex-A53 errata.
ERRATA_A53_836870		:=	1
ERRATA_A53_843419		:=	1
ERRATA_A53_855873		:=	1
