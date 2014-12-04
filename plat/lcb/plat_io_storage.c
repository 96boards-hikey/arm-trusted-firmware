/*
 * Copyright (c) 2014, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2014, Hisilicon Ltd.
 * Copyright (c) 2014, Linaro Ltd.
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

#include <assert.h>
#include <debug.h>
#include <io_driver.h>
#include <io_fip.h>
#include <io_dw_mmc.h>
#include <io_storage.h>
#include <platform_def.h>
#include <semihosting.h>	/* For FOPEN_MODE_... */
#include <string.h>

static const io_dev_connector_t *fip_dev_con;
static uintptr_t fip_dev_spec;
static uintptr_t fip_dev_handle;
static const io_dev_connector_t *dw_mmc_dev_con;
static uintptr_t dw_mmc_dev_spec;
static uintptr_t dw_mmc_dev_handle;
static uintptr_t dw_mmc_init_params;

static const io_block_spec_t fip_block_spec = {
	.offset = MMC_BASE,
	.length = MMC_SIZE
};

static const io_file_spec_t bl2_file_spec = {
	.path = BL2_IMAGE_NAME,
	.mode = FOPEN_MODE_RB
};

static int open_fip(const uintptr_t spec);
static int open_dw_mmc(const uintptr_t spec);

struct plat_io_policy {
	const char *image_name;
	uintptr_t *dev_handle;
	uintptr_t image_spec;
	int (*check)(const uintptr_t spec);
};

static const struct plat_io_policy policies[] = {
	{
		FIP_IMAGE_NAME,
		&dw_mmc_dev_handle,
		(uintptr_t)&fip_block_spec,
		open_dw_mmc
	}, {
		BL2_IMAGE_NAME,
		&fip_dev_handle,
		(uintptr_t)&bl2_file_spec,
		open_fip
	}, {
		0, 0, 0
	}
};

static int open_fip(const uintptr_t spec)
{
	int result = IO_FAIL;

	/* See if a Firmware Image Package is available */
	result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_NAME);
	if (result == IO_SUCCESS) {
		INFO("Using FIP\n");
		/*TODO: Check image defined in spec is present in FIP. */
	}
	return result;
}


static int open_dw_mmc(const uintptr_t spec)
{
	int result = IO_FAIL;
	uintptr_t local_image_handle;

	result = io_dev_init(dw_mmc_dev_handle, dw_mmc_init_params);
	if (result == IO_SUCCESS) {
		result = io_open(dw_mmc_dev_handle, spec, &local_image_handle);
		if (result == IO_SUCCESS) {
			/* INFO("Using DW MMC IO\n"); */
			io_close(local_image_handle);
		}
	}
	return result;
}

void io_setup(void)
{
	int io_result = IO_FAIL;

	/* Register the IO devices on this platform */
	io_result = register_io_dev_fip(&fip_dev_con);
	assert(io_result == IO_SUCCESS);

	io_result = register_io_dev_dw_mmc(&dw_mmc_dev_con);
	assert(io_result == IO_SUCCESS);

	/* Open connections to devices and cache the handles */
	io_result = io_dev_open(fip_dev_con, fip_dev_spec, &fip_dev_handle);
	assert(io_result == IO_SUCCESS);

	io_result = io_dev_open(dw_mmc_dev_con, dw_mmc_dev_spec,
				&dw_mmc_dev_handle);
	assert(io_result == IO_SUCCESS);

	/* Ignore improbable errors in release builds */
	(void)io_result;
}

/* Return an IO device handle and specification which can be used to access
 * an image. Use this to enforce platform load policy */
int plat_get_image_source(const char *image_name, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	int result = IO_FAIL;
	const struct plat_io_policy *policy;

	NOTICE("#%s, %d, img name:%s\n", __func__, __LINE__, image_name);
	if ((image_name != NULL) && (dev_handle != NULL) &&
	    (image_spec != NULL)) {
		policy = policies;
		while (policy->image_name != NULL) {
			if (strcmp(policy->image_name, image_name) == 0) {
				result = policy->check(policy->image_spec);
				if (result == IO_SUCCESS) {
					*image_spec = policy->image_spec;
					*dev_handle = *(policy->dev_handle);
					break;
				}
			}
			policy++;
		}
	} else {
		result = IO_FAIL;
	}
	return result;
}
