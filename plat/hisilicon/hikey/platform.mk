#
# Copyright (c) 2014-2016, Linaro Ltd. All rights reserved.
# Copyright (c) 2014-2016, Hisilicon Ltd.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# Neither the name of ARM nor the names of its contributors may be used
# to endorse or promote products derived from this software without specific
# prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

CONSOLE_BASE		:=	PL011_UART3_BASE
CRASH_CONSOLE_BASE	:=	PL011_UART3_BASE
PLAT_PL061_MAX_GPIOS	:=	160

# Process flags
$(eval $(call add_define,CONSOLE_BASE))
$(eval $(call add_define,CRASH_CONSOLE_BASE))
$(eval $(call add_define,PLAT_PL061_MAX_GPIOS))

ENABLE_PLAT_COMPAT	:=	0

USE_COHERENT_MEM	:=	1

PLAT_INCLUDES		:=	-Iinclude/common/tbbr			\
				-Iinclude/drivers/synopsys		\
				-Iplat/hisilicon/hikey/include

PLAT_BL_COMMON_SOURCES	:=	drivers/arm/pl011/pl011_console.S	\
				lib/aarch64/xlat_tables.c		\
				plat/common/aarch64/plat_common.c	\
				plat/hisilicon/hikey/aarch64/hikey_common.c

BL1_SOURCES		+=	bl1/tbbr/tbbr_img_desc.c		\
				drivers/arm/pl061/pl061_gpio.c		\
				drivers/arm/sp804/sp804_delay_timer.c	\
				drivers/delay_timer/delay_timer.c	\
				drivers/gpio/gpio.c			\
				drivers/io/io_block.c			\
				drivers/io/io_fip.c			\
				drivers/io/io_storage.c			\
				drivers/mmc/mmc.c			\
				drivers/synopsys/mmc/dw_mmc.c		\
				lib/cpus/aarch64/cortex_a53.S		\
				plat/common/aarch64/platform_up_stack.S	\
				plat/hisilicon/hikey/aarch64/hikey_helpers.S \
				plat/hisilicon/hikey/hikey_bl1_pll.c	\
				plat/hisilicon/hikey/hikey_bl1_setup.c	\
				plat/hisilicon/hikey/hikey_io_storage.c

BL2U_SOURCES		+=	drivers/io/io_block.c			\
				drivers/io/io_fip.c			\
				drivers/io/io_storage.c			\
				drivers/mmc/mmc.c			\
				drivers/synopsys/mmc/dw_mmc.c		\
				plat/common/aarch64/platform_up_stack.S	\
				plat/hisilicon/hikey/hikey_bl2u_setup.c	\
				plat/hisilicon/hikey/hikey_io_storage.c

BL2_SOURCES		+=	drivers/arm/sp804/sp804_delay_timer.c	\
				drivers/delay_timer/delay_timer.c	\
				drivers/io/io_block.c			\
				drivers/io/io_fip.c			\
				drivers/io/io_storage.c			\
				drivers/mmc/mmc.c			\
				drivers/synopsys/mmc/dw_mmc.c		\
				plat/common/aarch64/platform_up_stack.S	\
				plat/hisilicon/hikey/hikey_bl2_setup.c	\
				plat/hisilicon/hikey/hikey_io_storage.c

HIKEY_GIC_SOURCES	:=	drivers/arm/gic/arm_gic.c		\
				drivers/arm/gic/common/gic_common.c	\
				drivers/arm/gic/v2/gicv2_main.c		\
				drivers/arm/gic/v2/gicv2_helpers.c	\
				plat/common/plat_gicv2.c

BL31_SOURCES		+=	lib/cpus/aarch64/cortex_a53.S		\
				plat/common/aarch64/platform_mp_stack.S	\
				plat/common/aarch64/plat_psci_common.c	\
				plat/hisilicon/hikey/aarch64/hikey_helpers.S \
				plat/hisilicon/hikey/hikey_bl31_setup.c	\
				plat/hisilicon/hikey/hikey_pm.c		\
				plat/hisilicon/hikey/hikey_topology.c	\
				${HIKEY_GIC_SOURCES}
