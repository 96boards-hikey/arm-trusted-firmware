/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <debug.h>
#include <delay_timer.h>
#include <errno.h>
#include <mmio.h>
#include <string.h>

static void fw_data_init(void)
{
	unsigned long value;
	unsigned int (*p)[4];

	value = (unsigned long) mmio_read_32((uintptr_t) HISI_DATA_HEAD_BASE);
	p = (unsigned int (*)[4]) value;
	if (p == NULL) {
		INFO("fw data int fail\n");
		return;
	}

	while ((*p)[0] && (*p)[2]) {
		memcpy((void *)(unsigned long)((*p)[0]),
		       (const void *)(unsigned long)((*p)[2]),
		       (*p)[3]);
		p++;
	}
}

int load_lpm3(void)
{
	INFO("start fw loading\n");

	fw_data_init();

	flush_dcache_range((uintptr_t)HISI_RESERVED_MEM_BASE,
			   HISI_RESERVED_MEM_SIZE);

	sev();
	sev();

	INFO("fw load success\n");

	return 0;
}
