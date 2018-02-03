/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __HISI_PWRC_H__
#define __HISI_PWRC_H__

void hisi_cpuidle_lock(unsigned int cluster, unsigned int core);
void hisi_cpuidle_unlock(unsigned int cluster, unsigned int core);
void hisi_set_cpuidle_flag(unsigned int cluster, unsigned int core);
void hisi_clear_cpuidle_flag(unsigned int cluster, unsigned int core);
void hisi_set_cpu_boot_flag(unsigned int cluster, unsigned int core);
void hisi_clear_cpu_boot_flag(unsigned int cluster, unsigned int core);
int hisi_cluster_is_powered_on(unsigned int cluster);
void hisi_enter_core_idle(unsigned int cluster, unsigned int core);
void hisi_enter_cluster_idle(unsigned int cluster, unsigned int core);
void hisi_enter_ap_suspend(unsigned int cluster, unsigned int core);
void hisi_set_entrypoint(unsigned int cluster, unsigned int core,
			 uintptr_t entrypoint);
void hisi_dma_resume(void);


/* pdc api */
void hisi_pdc_mask_cluster_wakeirq(unsigned int cluster);
int hisi_test_pwrdn_allcores(unsigned int cluster, unsigned int core);
void hisi_disable_pdc(unsigned int cluster);
void hisi_enable_pdc(unsigned int cluster);
void hisi_powerup_core(unsigned int cluster, unsigned int core);
void hisi_powerdn_core(unsigned int cluster, unsigned int core);
void hisi_powerup_cluster(unsigned int cluster, unsigned int core);
void hisi_powerdn_cluster(unsigned int cluster, unsigned int core);
unsigned int hisi_test_cpu_down(unsigned int cluster, unsigned int core);

#endif /* __HISI_PWRC_H__ */
