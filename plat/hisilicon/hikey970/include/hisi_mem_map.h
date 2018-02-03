/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __HISI_MEM_MAP__
#define __HISI_MEM_MAP__

#define DDR_BASE			0x0
#define DDR_SIZE			0xC0000000

#define DEVICE_BASE			0xE0000000
#define DEVICE_SIZE			0x20000000

#define HISI_DATA_HEAD_BASE		(0xFFF0A408)

#define HISI_RESERVED_MEM_BASE		(0x8E200000)
#define HISI_RESERVED_MEM_SIZE		(0x00040000)

/* Memory location options for TSP */
#define HISI_SRAM_ID	0
#define HISI_DRAM_ID	1

/*
 * DDR for OP-TEE (32MB from 0x3E00000-0x3FFFFFFF) is divided in several
 * regions:
 *   - Secure DDR (default is the top 16MB) used by OP-TEE
 *   - Non-secure DDR used by OP-TEE (shared memory and padding) (4MB)
 *   - Secure DDR (4MB aligned on 4MB) for OP-TEE's "Secure Data Path" feature
 *   - Non-secure DDR (8MB) reserved for OP-TEE's future use
 */
#define DDR_SEC_SIZE			0x01000000
#define DDR_SEC_BASE			0x3F000000

#define DDR_SDP_SIZE			0x00400000
#define DDR_SDP_BASE			(DDR_SEC_BASE - 0x400000) /* align */

/*
 * Platform memory map related constants
 */

/*
 * BL1 specific defines.
 */
#define BL1_RO_BASE			(0x16800000)
#define BL1_RO_LIMIT			(BL1_RO_BASE + 0x10000)
#define BL1_RW_BASE			(BL1_RO_LIMIT)		/* 1681_0000 */
#define BL1_RW_SIZE			(0x00288000)
#define BL1_RW_LIMIT			(0x16D00000)

/*
 * BL2 specific defines.
 */
#define BL2_BASE			(BL1_RW_BASE + 0x8000)	/* 1681_8000 */
#define BL2_LIMIT			(BL2_BASE + 0x40000)	/* 1685_8000 */

/*
 * BL31 specific defines.
 */
#define BL31_BASE			(BL2_LIMIT)		/* 1685_8000 */
#define BL31_LIMIT			(BL31_BASE + 0x40000)	/* 1689_8000 */

/*
 * BL3-2 specific defines.
 */

/*
 * The TSP currently executes from TZC secured area of DRAM.
 */
#define BL32_DRAM_BASE                  DDR_SEC_BASE
#define BL32_DRAM_LIMIT                 (DDR_SEC_BASE+DDR_SEC_SIZE)

#if LOAD_IMAGE_V2
#ifdef SPD_opteed
/* Load pageable part of OP-TEE at end of allocated DRAM space for BL32 */
#define HISI_OPTEE_PAGEABLE_LOAD_BASE	(BL32_DRAM_LIMIT - \
					 HISI_OPTEE_PAGEABLE_LOAD_SIZE)
					/* 0x3FC0_0000 */
#define HISI_OPTEE_PAGEABLE_LOAD_SIZE	0x400000 /* 4MB */
#endif
#endif

#if (HISI_TSP_RAM_LOCATION_ID == HISI_DRAM_ID)
#define TSP_SEC_MEM_BASE		BL32_DRAM_BASE
#define TSP_SEC_MEM_SIZE		(BL32_DRAM_LIMIT - BL32_DRAM_BASE)
#define BL32_BASE			BL32_DRAM_BASE
#define BL32_LIMIT			BL32_DRAM_LIMIT
#elif (HISI_TSP_RAM_LOCATION_ID == HISI_SRAM_ID)
#error "SRAM storage of TSP payload is currently unsupported"
#else
#error "Currently unsupported HISI_TSP_LOCATION_ID value"
#endif

/* BL32 is mandatory in AArch32 */
#ifndef AARCH32
#ifdef SPD_none
#undef BL32_BASE
#endif /* SPD_none */
#endif

#define NS_BL1U_BASE			(BL31_LIMIT)		/* 1689_8000 */
#define NS_BL1U_SIZE			(0x00200000)
#define NS_BL1U_LIMIT			(NS_BL1U_BASE + NS_BL1U_SIZE)

#define HISI_NS_IMAGE_OFFSET	(BL2_BASE)	/* offset in l-loader */
#define HISI_NS_TMP_OFFSET		(0x16B00000)

#define SCP_BL2_BASE			(0x8E200000)
#define SCP_BL2_SIZE			(0x00040000)

#define UFS_BASE			0
/* FIP partition */
#define HISI_FIP_BASE		(UFS_BASE + 0x1400000)
#define HISI_FIP_MAX_SIZE		(12 << 20)

#define HISI_UFS_DESC_BASE		0x20000000
#define HISI_UFS_DESC_SIZE		0x00200000	/* 2MB */
#define HISI_UFS_DATA_BASE		0x10000000
#define HISI_UFS_DATA_SIZE		0x06000000	/* 96MB */

#endif /* __HISI_MEM_MAP__ */
