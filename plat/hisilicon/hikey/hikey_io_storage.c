/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <arch_helpers.h>
#include <assert.h>
#include <debug.h>
#include <firmware_image_package.h>
#include <io_block.h>
#include <io_driver.h>
#include <io_fip.h>
#include <io_memmap.h>
#include <io_storage.h>
#include <mmc.h>
#include <mmio.h>
#include <platform_def.h>
#include <semihosting.h>	/* For FOPEN_MODE_... */
#include <string.h>
#include "hikey_private.h"

struct plat_io_policy {
	uintptr_t *dev_handle;
	uintptr_t image_spec;
	int (*check)(const uintptr_t spec);
};

static const io_dev_connector_t *emmc_dev_con;
static uintptr_t emmc_dev_handle, emmc_rpmb_dev_handle;
static const io_dev_connector_t *fip_dev_con;
static uintptr_t fip_dev_handle;

static int check_emmc(const uintptr_t spec);
static int check_fip(const uintptr_t spec);

static io_block_entity_t emmc_fip_entity = {
	.region	= {
		/* updated by protocol */
		.offset	= HIKEY_FIP_BASE,
		.length	= HIKEY_FIP_MAX_SIZE
	},
	.ops	= {
		.read	= mmc_read_blocks,
		.write	= mmc_write_blocks,
	},
	.file_pos	= 0,
	.block_size	= MMC_BLOCK_SIZE,
	.lba		= 0,
	.rpmb_enable	= 0
};

static io_block_entity_t emmc_rpmb_entity = {
	.region	= {
		.offset	= HIKEY_NSBL1U_BASE,
		.length	= HIKEY_NSBL1U_MAX_SIZE
	},
	.ops	= {
		.read	= mmc_rpmb_read_blocks,
		.write	= mmc_rpmb_write_blocks,
	},
	.file_pos	= 0,
	.block_size	= MMC_BLOCK_SIZE,
	.lba		= 0,
	.rpmb_enable	= 1
};

/* It's used as temp buffer in block driver. */
static const io_block_spec_t io_block_mmc_spec = {
	.offset		= HIKEY_MMC_DATA_BASE,
	.length		= HIKEY_MMC_DATA_SIZE
};

static const io_uuid_spec_t bl2_uuid_spec = {
	.uuid = UUID_TRUSTED_BOOT_FIRMWARE_BL2,
};

static const io_uuid_spec_t bl31_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
};

static const io_uuid_spec_t bl33_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
};

static const struct plat_io_policy policies[] = {
	[FIP_IMAGE_ID] = {
		&emmc_dev_handle,
		(uintptr_t)&emmc_fip_entity,
		check_emmc
	},
	[BL2_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl2_uuid_spec,
		check_fip
	},
	[BL31_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl31_uuid_spec,
		check_fip
	},
	[BL33_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl33_uuid_spec,
		check_fip
	},
	[NS_BL1U_IMAGE_ID] = {
		&emmc_rpmb_dev_handle,
		(uintptr_t)&emmc_rpmb_entity,
		check_emmc
	}
};

static int check_emmc(const uintptr_t spec)
{
	int result;
	uintptr_t local_handle;

	result = io_dev_init(emmc_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(emmc_dev_handle, spec, &local_handle);
		if (result == 0) {
			io_close(local_handle);
		}
	}
	return result;
}

static int check_fip(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	/* See if a Firmware Image Package is available */
	result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);
	if (result == 0) {
		result = io_open(fip_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			VERBOSE("Using FIP\n");
			io_close(local_image_handle);
		}
	}
	return result;
}

void hikey_io_setup(void)
{
	int result;

	result = register_io_dev_block(&emmc_dev_con);
	assert(result == 0);

	result = register_io_dev_fip(&fip_dev_con);
	assert(result == 0);

	result = io_dev_open(emmc_dev_con, (uintptr_t)&io_block_mmc_spec,
			     &emmc_dev_handle);
	assert(result == 0);

	result = io_dev_open(fip_dev_con, (uintptr_t)NULL, &fip_dev_handle);
	assert(result == 0);

	/* Ignore improbable errors in release builds */
	(void)result;
}

/* Return an IO device handle and specification which can be used to access
 * an image. Use this to enforce platform load policy
 */
int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	int result;
	const struct plat_io_policy *policy;

	assert(image_id < ARRAY_SIZE(policies));

	policy = &policies[image_id];
	result = policy->check(policy->image_spec);
	assert(result == 0);

	*image_spec = policy->image_spec;
	*dev_handle = *(policy->dev_handle);

	return result;
}
