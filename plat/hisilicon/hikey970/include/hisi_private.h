/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __HISI_PRIVATE_H__
#define __HISI_PRIVATE_H__

#include <bl_common.h>

/*
 * Function and variable prototypes
 */
void hisi_init_mmu_el1(unsigned long total_base,
			unsigned long total_size,
			unsigned long ro_start,
			unsigned long ro_limit,
			unsigned long coh_start,
			unsigned long coh_limit);
void hisi_init_mmu_el3(unsigned long total_base,
			unsigned long total_size,
			unsigned long ro_start,
			unsigned long ro_limit,
			unsigned long coh_start,
			unsigned long coh_limit);
void hisi_init_ufs(void);
void hisi_io_setup(void);
void clr_ex(void);
void nop(void);

extern int load_lpm3(void);

#endif /* __HISI_PRIVATE_H__ */
