/*
 * ip_regulator.c
 */
#include <debug.h>
#include <runtime_svc.h>
#include <arch_helpers.h>
#include <stdint.h>
#include <uuid.h>
#include <platform_def.h>
#include <platform.h>
#include <hisi_service.h>
#include <assert.h>

enum ip_regulator_ops{
    IP_REGULATOR_DISABLE = 0,
    IP_REGULATOR_ENABLE,
    IP_REGULATOR_STATE,
};
static const plat_regulator_ops_t *g_plat_regulator_ops;
static uint64_t set_ip_regulator_power_up(uint64_t dev_id)
{
    if (NULL == g_plat_regulator_ops || NULL == g_plat_regulator_ops->set_regulator_power_up) {
        return (uint64_t)REGULATOR_E_ERROR;
    }
    return g_plat_regulator_ops->set_regulator_power_up(dev_id);
}

static uint64_t set_ip_regulator_power_down(uint64_t dev_id)
{
    if (NULL == g_plat_regulator_ops || NULL == g_plat_regulator_ops->set_regulator_power_down) {
        return (uint64_t)REGULATOR_E_ERROR;
    }
    return g_plat_regulator_ops->set_regulator_power_down(dev_id);
}

static uint64_t get_ip_regulator_power_state(uint64_t dev_id)
{
    if (NULL == g_plat_regulator_ops || NULL == g_plat_regulator_ops->get_regulator_power_state) {
        return (uint64_t)REGULATOR_E_ERROR;
    }
    return g_plat_regulator_ops->get_regulator_power_state(dev_id);
}

void regulator_setup(void)
{
    g_plat_regulator_ops = NULL;
    platform_regulator_ops_register(&g_plat_regulator_ops);
    assert(g_plat_regulator_ops);
    return;
}
/*******************************************************************************
 * IP REGULATOR handler for servicing SMCs.
 ******************************************************************************/

uint64_t ip_regulator_smc_handler(uint32_t smc_fid,
            uint64_t x1,
            uint64_t x2,
            uint64_t x3,
            uint64_t x4,
            void *cookie,
            void *handle,
            uint64_t flags)
{

    switch (x2) {
    case IP_REGULATOR_DISABLE:
        SMC_RET1(handle, set_ip_regulator_power_down(x1));
        break;
    case IP_REGULATOR_ENABLE:
        SMC_RET1(handle, set_ip_regulator_power_up(x1));
        break;
    case IP_REGULATOR_STATE:
        SMC_RET1(handle, get_ip_regulator_power_state(x1));
        break;
    default:
        break;
    }

    SMC_RET1(handle, SMC_UNK);
}
