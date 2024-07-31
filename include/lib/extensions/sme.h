/*
 * Copyright (c) 2021-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SME_H
#define SME_H

#include <stdlib.h> /* for rand() */

#define MAX_VL			(512)
#define MAX_VL_B		(MAX_VL / 8)
#define SME_SVQ_ARCH_MAX	(MASK(SMCR_ELX_LEN) >> SMCR_ELX_LEN_SHIFT)

/* convert SME SVL in bytes to SVQ */
#define SME_SVL_TO_SVQ(svl_bytes)		(((svl_bytes) >> 4U) - 1U)

/* get a random Streaming SVE VQ b/w 0 to SME_SVQ_ARCH_MAX */
#define SME_GET_RANDOM_SVQ	(rand() % (SME_SVQ_ARCH_MAX + 1))

typedef enum {
	SMSTART,	/* enters streaming sve mode and enables SME ZA array */
	SMSTART_SM,	/* enters streaming sve mode only */
	SMSTART_ZA,	/* enables SME ZA array storage only */
} smestart_instruction_type_t;

typedef enum {
	SMSTOP,		/* exits streaming sve mode, & disables SME ZA array */
	SMSTOP_SM,	/* exits streaming sve mode only */
	SMSTOP_ZA,	/* disables SME ZA array storage only */
} smestop_instruction_type_t;

/* SME feature related prototypes. */
void sme_smstart(smestart_instruction_type_t smstart_type);
void sme_smstop(smestop_instruction_type_t smstop_type);
uint32_t sme_probe_svl(uint8_t sme_max_svq);

/* Assembly function prototypes. */
uint64_t sme_rdsvl_1(void);
void sme_try_illegal_instruction(void);
void sme_vector_to_ZA(const uint64_t *input_vector);
void sme_ZA_to_vector(const uint64_t *output_vector);
void sme2_load_zt0_instruction(const uint64_t *inputbuf);
void sme2_store_zt0_instruction(const uint64_t *outputbuf);
void sme_config_svq(uint32_t svq);
void sme_enable_fa64(void);
void sme_disable_fa64(void);
bool sme_smstat_sm(void);
bool sme_feat_fa64_enabled(void);

#endif /* SME_H */
