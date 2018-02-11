#ifndef HISI_SERVICE_H_
#define HISI_SERVICE_H_

#include <stdio.h>

/****** all hisi IP REGULATOR module service function id start *****/
#define IP_REGULATOR_FID_MASK           0xfffffff0u
#define IP_REGULATOR_FID_VALUE          0xc500fff0u
/****** all hisi IP REGULATOR module service function id end *****/

/* The macros below are used to identify IP REGULATOR calls from the SMC function ID */
#define is_ip_regulator_fid(_fid) \
    (((_fid) & IP_REGULATOR_FID_MASK) == IP_REGULATOR_FID_VALUE)

/*******************************************************************************
 * regulator error codes
 ******************************************************************************/

#define REGULATOR_E_SUCCESS           (0)
#define REGULATOR_E_ERROR             (-1)
typedef struct plat_regulator_ops {
	uint64_t (*set_regulator_power_up)(uint64_t dev_id);
	uint64_t (*set_regulator_power_down)(uint64_t dev_id);
	uint64_t (*get_regulator_power_state)(uint64_t dev_id);
} plat_regulator_ops_t;

void regulator_setup(void);
uint64_t ip_regulator_smc_handler(uint32_t smc_fid,
              uint64_t x1,
              uint64_t x2,
              uint64_t x3,
              uint64_t x4,
              void *cookie,
              void *handle,
              uint64_t flags);

int platform_regulator_ops_register(const struct plat_regulator_ops **);

#endif /* HISI_SERVICE_H_ */
