/*
 * std_hisi_service_setup.c
 *
 */
#include <assert.h>
#include <cpu_data.h>
#include <debug.h>
#include <pmf.h>
#include <psci.h>
#include <runtime_instr.h>
#include <runtime_svc.h>
#include <smcc_helpers.h>
#include <std_svc.h>
#include <stdint.h>
#include <uuid.h>
#include <hisi_service.h>


/* Standard Service UUID */
/*
DEFINE_SVC_UUID(hisi_service_svc_uid,
        0x108d905b, 0xf863, 0x47e8, 0xae, 0x2d,
        0xc0, 0xfb, 0x56, 0x41, 0xf6, 0xe2);
*/
/* Setup Standard Services */
static int32_t std_smc_hisi_service_setup(void)
{
	regulator_setup();
	return 0;
}


/*
 * Top-level Standard Service SMC handler. This handler will in turn dispatch
 * calls to EFUSE SMC handler
 */
uint64_t std_smc_hisi_service_handler(uint32_t smc_fid,
                 uint64_t x1,
                 uint64_t x2,
                 uint64_t x3,
                 uint64_t x4,
                 void *cookie,
                 void *handle,
                 uint64_t flags)
{
     /*
     * Dispatch ip regulator calls to ACCESS_REGISTER SMC handler and return its return
     * value
     */
     uint64_t ret;

    if (is_ip_regulator_fid(smc_fid)) {

	ret = ip_regulator_smc_handler(smc_fid, x1, x2, x3, x4, cookie, handle, flags);
	SMC_RET0(ret);
    }
	SMC_RET1(handle, SMC_UNK);
}

DECLARE_RT_SVC(
        std_smc_hisi_service,

        OEN_STD_HISI_SERVICE_START,
        OEN_STD_HISI_SERVICE_END,
        SMC_TYPE_FAST,
        std_smc_hisi_service_setup,
        std_smc_hisi_service_handler
);
