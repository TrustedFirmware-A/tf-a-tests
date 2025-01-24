/*
 * Copyright (c) 2022-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <string.h>

#include <arch_features.h>

#include <debug.h>
#include <heap/page_alloc.h>
#include <test_helpers.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_realm_rmi.h>
#include <host_shared_data.h>
#include <plat/common/platform.h>
#include <realm_def.h>
#include <tftf_lib.h>
#include <utils_def.h>

#define SET_ARG(_n) {			\
	case _n:			\
	regs[_n] = rand64();		\
	CONC(args->arg, _n) = regs[_n];	\
	__attribute__((fallthrough));	\
}

#define	CHECK_RET(_n) {					\
	if (CONC(ret_val.ret, _n) != regs[_n]) {	\
		cmp_flag |= (1U << _n);			\
	}						\
}

static bool rmi_cmp_result;
static unsigned short vmid;
static spinlock_t pool_lock;
static unsigned int pool_counter;

static smc_ret_values host_rmi_handler(smc_args *args, unsigned int in_reg)
{
	u_register_t regs[8];
	smc_ret_values ret_val;
	unsigned int cmp_flag = 0U;

	assert(args != NULL);
	assert((in_reg >= 1U) && (in_reg <= 7U));

	/* Function identifier */
	regs[0] = (u_register_t)args->fid;

	/* X4 and X5 can be passed as parameters */
	regs[4] = args->arg4;
	regs[5] = args->arg5;

	/* SMC calls arguments in X1-X7 */
	switch (in_reg) {
		SET_ARG(1);
		SET_ARG(2);
		SET_ARG(3);
		SET_ARG(4);
		SET_ARG(5);
		SET_ARG(6);
	default:
		regs[7] = rand();
		args->arg7 = regs[7];
	}

	ret_val = tftf_smc(args);

	/*
	 * According to SMCCC v1.2 X4-X7 registers' values
	 * must be preserved unless they contain result,
	 * as specified in the function definition.
	 */
	if ((regs[0] != SMC_RMI_RTT_READ_ENTRY) &&
	    (regs[0] != SMC_RMI_RTT_AUX_MAP_PROTECTED)) {
		CHECK_RET(4);
	}

	CHECK_RET(5);
	CHECK_RET(6);
	CHECK_RET(7);

	if (cmp_flag != 0U) {
		rmi_cmp_result = false;

		ERROR("RMI SMC 0x%lx corrupted registers: %s %s %s %s\n",
			regs[0],
			(((cmp_flag & (1U << 4)) != 0U) ? "X4" : ""),
			(((cmp_flag & (1U << 5)) != 0U) ? "X5" : ""),
			(((cmp_flag & (1U << 6)) != 0U) ? "X6" : ""),
			(((cmp_flag & (1U << 7)) != 0U) ? "X7" : ""));
	}

	return ret_val;
}

void host_rmi_init_cmp_result(void)
{
	rmi_cmp_result = true;
}

bool host_rmi_get_cmp_result(void)
{
	return rmi_cmp_result;
}

u_register_t host_rmi_psci_complete(u_register_t calling_rec, u_register_t target_rec,
		unsigned long status)
{
	return (host_rmi_handler(&(smc_args) {SMC_RMI_PSCI_COMPLETE, calling_rec,
				target_rec, status}, 4U)).ret0;
}

u_register_t host_rmi_data_create(bool unknown,
				  u_register_t rd,
				  u_register_t data,
				  u_register_t map_addr,
				  u_register_t src)
{
	if (unknown) {
		return host_rmi_handler(&(smc_args) {SMC_RMI_DATA_CREATE_UNKNOWN,
					rd, data, map_addr}, 4U).ret0;
	} else {
		return host_rmi_handler(&(smc_args) {SMC_RMI_DATA_CREATE,
					/* X5 = flags */
					rd, data, map_addr, src, 0UL}, 6U).ret0;
	}
}

static inline u_register_t host_rmi_realm_activate(u_register_t rd)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_REALM_ACTIVATE,
					     rd}, 2U).ret0;
}

u_register_t host_rmi_realm_create(u_register_t rd, u_register_t params_ptr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_REALM_CREATE, rd, params_ptr},
						3U).ret0;
}

u_register_t host_rmi_realm_destroy(u_register_t rd)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_REALM_DESTROY, rd},
				2U).ret0;
}

u_register_t host_rmi_data_destroy(u_register_t rd,
				   u_register_t map_addr,
				   u_register_t *data,
				   u_register_t *top)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_DATA_DESTROY, rd, map_addr,
						(u_register_t)&rets}, 4U);

	*data = rets.ret1;
	*top = rets.ret2;
	return rets.ret0;
}

static inline u_register_t host_rmi_rec_create(u_register_t rd,
						u_register_t rec,
						u_register_t params_ptr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_REC_CREATE,
				rd, rec, params_ptr}, 4U).ret0;
}

static inline u_register_t host_rmi_rec_destroy(u_register_t rec)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_REC_DESTROY, rec}, 2U).ret0;
}

static inline u_register_t host_rmi_rtt_create(u_register_t rd,
						u_register_t rtt,
						u_register_t map_addr,
						long  level)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_RTT_CREATE,
				rd, rtt, map_addr, (u_register_t)level}, 5U).ret0;
}

static inline u_register_t host_rmi_rtt_aux_create(u_register_t rd,
						u_register_t rtt,
						u_register_t map_addr,
						long  level,
						u_register_t tree_index)
{
	return host_rmi_handler(&(smc_args) {RMI_RTT_AUX_CREATE,
				rd, rtt, map_addr, (u_register_t)level, tree_index}, 6U).ret0;
}

u_register_t host_rmi_rtt_destroy(u_register_t rd,
				  u_register_t map_addr,
				  long level,
				  u_register_t *rtt,
				  u_register_t *top)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_RTT_DESTROY,
				rd, map_addr, (u_register_t)level,
				(u_register_t)&rets}, 5U);
	*rtt = rets.ret1;
	*top = rets.ret2;
	return rets.ret0;
}

u_register_t host_rmi_rtt_aux_destroy(u_register_t rd,
				  u_register_t map_addr,
				  long level,
				  u_register_t tree_index,
				  u_register_t *rtt,
				  u_register_t *top)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {RMI_RTT_AUX_DESTROY,
				rd, map_addr, (u_register_t)level,
				tree_index,
				(u_register_t)&rets}, 6U);
	*rtt = rets.ret1;
	*top = rets.ret2;
	return rets.ret0;
}

u_register_t host_rmi_features(u_register_t index, u_register_t *features)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_FEATURES, index}, 2U);
	*features = rets.ret1;
	return rets.ret0;
}

u_register_t host_rmi_rtt_init_ripas(u_register_t rd,
				   u_register_t start,
				   u_register_t end,
				   u_register_t *top)

{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_RTT_INIT_RIPAS,
				rd, start, end}, 4U);
	*top = rets.ret1;
	return rets.ret0;
}

/*
 * RMI_RTT_FOLD destroys a homogeneous child RTT,
 * and moves information which was stored in the child RTT into the parent RTTE.
 * Input rd, base adr of IPA range in RTT, level of RTT
 * Output result, base PA of child RTT which was destroyed
 */
static inline u_register_t host_rmi_rtt_fold(u_register_t rd,
					     u_register_t map_addr,
					     long level,
					     u_register_t *pa)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_RTT_FOLD,
				rd, map_addr, (u_register_t)level,
				(u_register_t)&rets}, 5U);
	*pa = rets.ret1;
	return rets.ret0;
}

static inline u_register_t host_rmi_rtt_aux_fold(u_register_t rd,
						 u_register_t map_addr,
						 long level,
						 u_register_t tree_index,
						 u_register_t *pa)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_RTT_AUX_FOLD,
				rd, map_addr, (u_register_t)level,
				tree_index,
				(u_register_t)&rets}, 6U);
	*pa = rets.ret1;
	return rets.ret0;
}

static inline u_register_t host_rmi_rec_aux_count(u_register_t rd,
						  u_register_t *aux_count)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_REC_AUX_COUNT, rd}, 2U);
	*aux_count = rets.ret1;
	return rets.ret0;
}

u_register_t host_rmi_rtt_set_ripas(u_register_t rd,
				    u_register_t rec,
				    u_register_t start,
				    u_register_t end,
				    u_register_t *top)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_RTT_SET_RIPAS,
				rd, rec, start, end}, 5U);
	*top = rets.ret1;
	return rets.ret0;
}

u_register_t host_rmi_rtt_mapunprotected(u_register_t rd,
					 u_register_t map_addr,
					 long level,
					 u_register_t ns_pa)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_RTT_MAP_UNPROTECTED,
				rd, map_addr, (u_register_t)level, ns_pa}, 5U).ret0;
}

u_register_t host_rmi_rtt_aux_map_unprotected(u_register_t rd,
					      u_register_t map_addr,
					      u_register_t tree_index,
					      u_register_t *fail_index,
					      u_register_t *level_pri,
					      u_register_t *state)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_RTT_AUX_MAP_UNPROTECTED,
				rd, map_addr, tree_index}, 4U);
	*fail_index = rets.ret1;
	*level_pri = rets.ret2;
	*state = rets.ret3;
	return rets.ret0;
}

u_register_t host_rmi_rtt_aux_map_protected(u_register_t rd,
					 u_register_t map_addr,
					 u_register_t tree_index,
					 u_register_t *fail_index,
					 u_register_t *level_pri,
					 u_register_t *state,
					 u_register_t *ripas)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_RTT_AUX_MAP_PROTECTED,
				rd, map_addr, tree_index}, 4U);
	*fail_index = rets.ret1;
	*level_pri = rets.ret2;
	*state = rets.ret3;
	*ripas = rets.ret4;
	return rets.ret0;
}

u_register_t host_rmi_rtt_readentry(u_register_t rd,
				   u_register_t map_addr,
				   long level,
				   struct rtt_entry *rtt)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_RTT_READ_ENTRY,
					rd, map_addr, (u_register_t)level}, 4U);
	rtt->walk_level = (long)rets.ret1;
	rtt->state = rets.ret2;
	rtt->out_addr = rets.ret3;
	rtt->ripas = rets.ret4;
	return rets.ret0;
}

u_register_t host_rmi_rtt_unmap_unprotected(u_register_t rd,
					  u_register_t map_addr,
					  long level,
					  u_register_t *top)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_RTT_UNMAP_UNPROTECTED,
					rd, map_addr, (u_register_t)level}, 4U);
	*top = rets.ret1;
	return rets.ret0;
}

u_register_t host_rmi_rtt_aux_unmap_unprotected(u_register_t rd,
					  u_register_t map_addr,
					  u_register_t tree_index,
					  u_register_t *top,
					  u_register_t *level)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_RTT_AUX_UNMAP_UNPROTECTED,
					rd, map_addr, tree_index}, 4U);
	*top = rets.ret1;
	*level = rets.ret2;
	return rets.ret0;
}

u_register_t host_rmi_rtt_aux_unmap_protected(u_register_t rd,
					  u_register_t map_addr,
					  u_register_t tree_index,
					  u_register_t *top,
					  u_register_t *level)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_RTT_AUX_UNMAP_PROTECTED,
					rd, map_addr, tree_index}, 4U);
	*top = rets.ret1;
	*level = rets.ret2;
	return rets.ret0;
}

bool host_ipa_is_ns(u_register_t addr, u_register_t rmm_feat_reg0)
{
	return (addr >> (EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ, rmm_feat_reg0) - 1UL) == 1UL);
}

static inline u_register_t host_realm_rtt_create(struct realm *realm,
						 u_register_t addr,
						 long level,
						 u_register_t phys)
{
	addr = ALIGN_DOWN(addr, RTT_MAP_SIZE(level - 1));
	return host_rmi_rtt_create(realm->rd, phys, addr, level);
}

u_register_t host_rmi_create_rtt_levels(struct realm *realm,
					u_register_t map_addr,
					long level, long max_level)
{
	u_register_t rtt, ret;

	while (level++ < max_level) {
		rtt = (u_register_t)page_alloc(PAGE_SIZE);
		if (rtt == HEAP_NULL_PTR) {
			ERROR("Failed to allocate memory for rtt\n");
			return REALM_ERROR;
		} else {
			ret = host_rmi_granule_delegate(rtt);
			if (ret != RMI_SUCCESS) {
				ERROR("%s() failed, rtt=0x%lx ret=0x%lx\n",
					"host_rmi_granule_delegate", rtt, ret);
				return REALM_ERROR;
			}
		}
		ret = host_realm_rtt_create(realm, map_addr, (u_register_t)level, rtt);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, rtt=0x%lx ret=0x%lx\n",
				"host_realm_rtt_create", rtt, ret);
			host_rmi_granule_undelegate(rtt);
			page_free(rtt);
			return REALM_ERROR;
		}
	}

	return REALM_SUCCESS;
}

static u_register_t host_realm_create_rtt_aux_levels(struct realm *realm,
					    u_register_t map_addr,
					    long level, long max_level,
					    u_register_t tree_index)
{
	u_register_t rtt, ret, ipa_align;

	assert(tree_index != PRIMARY_RTT_INDEX);
	while (level++ < max_level) {
		rtt = (u_register_t)page_alloc(PAGE_SIZE);
		if (rtt == HEAP_NULL_PTR) {
			ERROR("Failed to allocate memory for rtt\n");
			return REALM_ERROR;
		} else {
			ret = host_rmi_granule_delegate(rtt);
			if (ret != RMI_SUCCESS) {
				ERROR("%s() failed, rtt=0x%lx ret=0x%lx\n",
					"host_rmi_granule_delegate", rtt, ret);
				return REALM_ERROR;
			}
		}
		ipa_align = ALIGN_DOWN(map_addr, RTT_MAP_SIZE(level <= 0 ? level : level - 1));
		ret = host_rmi_rtt_aux_create(realm->rd, rtt, ipa_align, level,
				tree_index);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, map_addr =0x%lx ipa_align=0x%lx level=%lx ret=0x%lx\n",
				"host_realm_rtt_aux_create", map_addr, ipa_align, level, ret);
			host_rmi_granule_undelegate(rtt);
			page_free(rtt);
			return REALM_ERROR;
		}
	}

	return REALM_SUCCESS;
}

u_register_t host_realm_fold_rtt(u_register_t rd, u_register_t addr,
				 long level)
{
	u_register_t pa, ret;

	ret = host_rmi_rtt_fold(rd, addr, level, &pa);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, addr=0x%lx ret=0x%lx\n",
			"host_rmi_rtt_fold", addr, ret);
		return REALM_ERROR;
	}

	/*
	 * RMI_RTT_FOLD returns PA of the RTT which was destroyed
	 * Corresponding IPA needs to be undelegated and freed
	 */
	ret = host_rmi_granule_undelegate(pa);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, rtt=0x%lx ret=0x%lx\n",
			"host_rmi_granule_undelegate", pa, ret);
		return REALM_ERROR;
	}
	page_free(pa);

	return REALM_SUCCESS;

}

u_register_t host_realm_aux_map_protected_data(struct realm *realm,
						u_register_t target_pa,
						u_register_t map_size,
						u_register_t tree_index,
						u_register_t *fail_index,
						u_register_t *level_pri,
						u_register_t *state,
						u_register_t *ripas)
{
	u_register_t ret, top;
	long level;
	int8_t ulevel;
	u_register_t size = 0UL;
	u_register_t map_addr = target_pa;

	assert(tree_index != PRIMARY_RTT_INDEX);
	for (size = 0UL; size < map_size; size += PAGE_SIZE) {
		ret =  host_rmi_rtt_aux_map_protected(realm->rd, map_addr, tree_index,
			fail_index, level_pri, state, ripas);

		if (RMI_RETURN_STATUS(ret) == RMI_ERROR_RTT_AUX) {
			/* Create missing RTTs till L3 and retry */
			ulevel = RMI_RETURN_INDEX(ret);
			level = (long)ulevel;

			ret = host_realm_create_rtt_aux_levels(realm, map_addr,
							 level,
							 3U, tree_index);
			if (ret != RMI_SUCCESS) {
				ERROR("%s() failed, ret=0x%lx line=%u\n",
					"host_realm_create_rtt_aux_levels",
					ret, __LINE__);
				goto err;
			}
			ret =  host_rmi_rtt_aux_map_protected(realm->rd, target_pa, tree_index,
				fail_index, level_pri, state, ripas);
			if (ret != RMI_SUCCESS) {
				ERROR("%s() failed, ret=0x%lx\n",
				"host_rmi_data_create", ret);
				goto err;
			}
		} else if (ret != RMI_SUCCESS) {
			ERROR("host_rmi_rtt_aux_map_protected failed ret = 0x%lx", ret);
			goto err;
		}
		map_addr += PAGE_SIZE;
	}
	return REALM_SUCCESS;

err:
	while (size >= PAGE_SIZE) {
		ret = host_rmi_rtt_aux_unmap_protected(realm->rd, target_pa, tree_index,
				&top, (u_register_t *)&level);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, addr=0x%lx ret=0x%lx\n",
				"host_rmi_rtt_aux_unmap_protected", map_addr, ret);
		}
		size -= PAGE_SIZE;
		map_addr -= PAGE_SIZE;
	}
	return REALM_ERROR;
}

u_register_t host_realm_aux_unmap_protected_data(struct realm *realm,
						u_register_t target_ipa,
						u_register_t map_size,
						u_register_t tree_index,
						u_register_t *top,
						u_register_t *level)
{
	u_register_t ret, size = 0UL, map_addr = target_ipa;

	assert(tree_index != PRIMARY_RTT_INDEX);
	for (size = 0UL; size < map_size; size += PAGE_SIZE) {
		ret = host_rmi_rtt_aux_unmap_protected(realm->rd, map_addr, tree_index,
				top, level);
		if (ret != RMI_SUCCESS) {
			WARN("%s() failed, ret=0x%lx line=%u\n",
					"host_rmi_rtt_aux_unmap_protected",
					ret, __LINE__);
			return REALM_ERROR;
		}
		map_addr += PAGE_SIZE;
	}
	return REALM_SUCCESS;
}

u_register_t host_realm_delegate_map_protected_data(bool unknown,
						    struct realm *realm,
						    u_register_t target_pa,
						    u_register_t map_size,
						    u_register_t src_pa)
{
	u_register_t rd = realm->rd;
	int8_t level;
	u_register_t ret = 0UL;
	u_register_t size = 0UL;
	u_register_t phys = target_pa;
	u_register_t map_addr = target_pa;

	if (!IS_ALIGNED(map_addr, PAGE_SIZE)) {
		return REALM_ERROR;
	}

	for (size = 0UL; size < map_size; size += PAGE_SIZE) {
		ret = host_rmi_granule_delegate(phys);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, PA=0x%lx ret=0x%lx\n",
				"host_rmi_granule_delegate", phys, ret);
			return REALM_ERROR;
		}

		ret = host_rmi_data_create(unknown, rd, phys, map_addr, src_pa);

		if (RMI_RETURN_STATUS(ret) == RMI_ERROR_RTT) {
			/* Create missing RTTs till L3 and retry */
			level = RMI_RETURN_INDEX(ret);
			ret = host_rmi_create_rtt_levels(realm, map_addr,
							 (u_register_t)level,
							 3U);
			if (ret != RMI_SUCCESS) {
				ERROR("%s() failed, ret=0x%lx line=%u\n",
					"host_rmi_create_rtt_levels",
					ret, __LINE__);
				goto err;
			}

			ret = host_rmi_data_create(unknown, rd, phys, map_addr,
						   src_pa);
		}

		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, ret=0x%lx\n",
				"host_rmi_data_create", ret);
			goto err;
		}

		phys += PAGE_SIZE;
		src_pa += PAGE_SIZE;
		map_addr += PAGE_SIZE;
	}

	return REALM_SUCCESS;

err:
	while (size >= PAGE_SIZE) {
		u_register_t data, top;

		ret = host_rmi_data_destroy(rd, map_addr, &data, &top);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, addr=0x%lx ret=0x%lx\n",
				"host_rmi_data_destroy", map_addr, ret);
		}

		ret = host_rmi_granule_undelegate(phys);
		if (ret != RMI_SUCCESS) {
			/* Page can't be returned to NS world so is lost */
			ERROR("%s() failed, ret=0x%lx\n",
				"host_rmi_granule_undelegate", ret);
		}
		phys -= PAGE_SIZE;
		size -= PAGE_SIZE;
		map_addr -= PAGE_SIZE;
	}

	return REALM_ERROR;
}

static u_register_t rtt_s2ap_set_pi_index(u_register_t s2tte, u_register_t pi_index)
{
	s2tte &= ~S2TTE_PI_INDEX_MASK;
	s2tte |= INPLACE(S2TTE_PI_INDEX_BIT0, pi_index & 1) |
		INPLACE(S2TTE_PI_INDEX_BIT1, (pi_index >> 1) & 1) |
		INPLACE(S2TTE_PI_INDEX_BIT2, (pi_index >> 2) & 1) |
		INPLACE(S2TTE_PI_INDEX_BIT3, (pi_index >> 3) & 1);
	return s2tte;
}

u_register_t host_realm_map_unprotected(struct realm *realm,
					u_register_t ns_pa,
					u_register_t map_size)
{
	u_register_t rd = realm->rd;
	long map_level;
	u_register_t desc;
	int8_t level;
	u_register_t ret = 0UL;
	u_register_t phys = ns_pa;
	u_register_t map_addr = ns_pa |
			(1UL << (EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ,
			realm->rmm_feat_reg0) - 1UL));

	if (!IS_ALIGNED(map_addr, map_size)) {
		return REALM_ERROR;
	}

	switch (map_size) {
	case PAGE_SIZE:
		map_level = 3UL;
		break;
	case RTT_L2_BLOCK_SIZE:
		map_level = 2UL;
		break;
	case RTT_L1_BLOCK_SIZE:
		map_level = 1UL;
		break;
	default:
		ERROR("Unknown map_size=0x%lx\n", map_size);
		return REALM_ERROR;
	}

	if ((realm->rmm_feat_reg0 & RMI_FEATURE_REGISTER_0_LPA2) != 0L) {
		desc = (phys & ~OA_50_51_MASK) |
				INPLACE(TTE_OA_50_51, EXTRACT(OA_50_51, phys));
	} else {
		desc = phys;
	}

	if (realm->rtt_s2ap_enc_indirect) {
		desc |= S2TTE_MEMATTR_FWB_NORMAL_WB;
		desc = rtt_s2ap_set_pi_index(desc, RMI_PERM_S2AP_RW_IDX);
	} else {
		desc |= S2TTE_ATTR_FWB_WB_RW;
	}

	ret = host_rmi_rtt_mapunprotected(rd, map_addr, map_level, desc);

	if (RMI_RETURN_STATUS(ret) == RMI_ERROR_RTT) {
		/* Create missing RTTs and retry */
		level = RMI_RETURN_INDEX(ret);
		ret = host_rmi_create_rtt_levels(realm, map_addr,
						 level, map_level);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, ret=0x%lx line=%u\n",
				"host_rmi_create_rtt_levels", ret, __LINE__);
			return REALM_ERROR;
		}

		ret = host_rmi_rtt_mapunprotected(rd, map_addr, map_level,
						  desc);
	}
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, ret=0x%lx\n", "host_rmi_rtt_mapunprotected",
			ret);
		return REALM_ERROR;
	}

	return REALM_SUCCESS;
}

static u_register_t host_realm_rtt_destroy(struct realm *realm,
					   u_register_t addr,
					   long level,
					   u_register_t *rtt,
					   u_register_t *top)
{
	addr = ALIGN_DOWN(addr, RTT_MAP_SIZE(level - 1L));
	return host_rmi_rtt_destroy(realm->rd, addr, level, rtt, top);
}

static u_register_t host_realm_destroy_free_rtt(struct realm *realm,
						u_register_t addr,
						long level,
						u_register_t rtt_granule)
{
	u_register_t rtt, top, ret;

	ret = host_realm_rtt_destroy(realm, addr, level, &rtt, &top);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, ret=0x%lx\n",
			"host_realm_rtt_destroy", ret);
		return REALM_ERROR;
	}

	ret = host_rmi_granule_undelegate(rtt_granule);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, rtt=0x%lx ret=0x%lx\n",
			"host_rmi_granule_undelegate", rtt_granule, ret);
		return REALM_ERROR;
	}

	page_free(rtt_granule);
	return REALM_SUCCESS;
}

static u_register_t host_realm_destroy_free_aux_rtt(struct realm *realm,
						    u_register_t addr,
						    long level)
{
	u_register_t rtt2, top, ret;

	/* Destroy and undelegate RTT entry for all trees */
	for (unsigned int tree_index = 1U;
			tree_index <= realm->num_aux_planes;
			tree_index++) {

		ret = host_rmi_rtt_aux_destroy(realm->rd, ALIGN_DOWN(addr,
					RTT_MAP_SIZE(level)),
					level + 1L,
					tree_index, &rtt2, &top);

		if (ret != RMI_SUCCESS) {
			/*
			 * IPA might not be mapped on all AUX RTTs
			 * ignore error
			 */
			VERBOSE("%s() failed, map_addr=0x%lx ret=0x%lx \
					rtt2=0x%lx \
					top=0x%lx level=0x%lx\n",
					"host_rmi_rtt_aux_destroy",
					addr, ret, rtt2,
					top, level + 1L);
		}

		if (rtt2 != 0UL) {
			ret = host_rmi_granule_undelegate(rtt2);
			if (ret != RMI_SUCCESS) {
				ERROR("%s() failed, rtt=0x%lx ret=0x%lx\n",
					"host_rmi_granule_undelegate",
					rtt2, ret);
				return REALM_ERROR;
			}

			page_free(rtt2);
		}
	}
	return REALM_SUCCESS;
}

static u_register_t host_realm_destroy_undelegate_range(struct realm *realm,
							u_register_t ipa,
							u_register_t addr,
							u_register_t size)
{
	u_register_t rd = realm->rd;
	u_register_t ret;
	u_register_t data, top;

	while (size >= PAGE_SIZE) {
		ret = host_rmi_data_destroy(rd, ipa, &data, &top);

		if (ret == RMI_ERROR_RTT_AUX) {
			/* Unmap from all Aux RTTs */
			for (unsigned int tree_index = 1U; tree_index <= realm->num_aux_planes;
					tree_index++) {
				u_register_t top1, level1;

				/* IPA might not be mapped in all Aux RTTs ignore error */
				ret = host_rmi_rtt_aux_unmap_protected(
								rd,
								ipa,
								tree_index,
								&top1, &level1);
				if (ret != RMI_SUCCESS) {
					VERBOSE("%s() failed, addr=0x%lx ret=0x%lx tree=0x%x\n",
					"host_rmi_rtt_aux_unmap_protected",
					ipa, ret, tree_index);
				}
			}
			/* Retry DATA_DESTROY */
			continue;
		}

		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, addr=0x%lx ret=0x%lx\n",
				"host_rmi_data_destroy", ipa, ret);
			return REALM_ERROR;
		}

		ret = host_rmi_granule_undelegate(addr);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, addr=0x%lx ret=0x%lx\n",
				"host_rmi_granule_undelegate", ipa, ret);
			return REALM_ERROR;
		}

		page_free(addr);

		addr += PAGE_SIZE;
		ipa += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	return REALM_SUCCESS;
}

static u_register_t host_realm_tear_down_rtt_range(struct realm *realm,
						   long level,
						   u_register_t start,
						   u_register_t end)
{
	u_register_t rd = realm->rd;
	u_register_t map_size = RTT_MAP_SIZE(level);
	u_register_t map_addr, next_addr, rtt_out_addr, end_addr, top;
	struct rtt_entry rtt;
	u_register_t ret;

	for (map_addr = start; map_addr < end; map_addr = next_addr) {
		next_addr = ALIGN(map_addr + 1U, map_size);
		end_addr = MIN(next_addr, end);

		ret = host_rmi_rtt_readentry(rd, ALIGN_DOWN(map_addr, map_size),
						level, &rtt);
		if (ret != RMI_SUCCESS) {
			continue;
		}

		rtt_out_addr = rtt.out_addr;

		switch (rtt.state) {
		case RMI_ASSIGNED:
			if (host_ipa_is_ns(map_addr, realm->rmm_feat_reg0)) {

				u_register_t level1;

				/* Unmap from all Aux RTT */
				if (!realm->rtt_s2ap_enc_indirect) {
					for (unsigned int tree_index = 1U;
						tree_index <= realm->num_aux_planes;
						tree_index++) {

						ret = host_rmi_rtt_aux_unmap_unprotected(
								rd,
								map_addr,
								tree_index,
								&top, &level1);

						if (ret != RMI_SUCCESS) {
							ERROR("%s() failed, addr=0x%lx ret=0x%lx\n",
							"host_rmi_rtt_unmap_unprotected",
							map_addr, ret);
						}
					}
				}

				ret = host_rmi_rtt_unmap_unprotected(
								rd,
								map_addr,
								level,
								&top);
				if (ret != RMI_SUCCESS) {
					ERROR("%s() failed, addr=0x%lx ret=0x%lx\n",
						"host_rmi_rtt_unmap_unprotected",
						map_addr, ret);
					return REALM_ERROR;
				}
			} else {
				ret = host_realm_destroy_undelegate_range(
									realm,
									map_addr,
									rtt_out_addr,
									map_size);
				if (ret != RMI_SUCCESS) {
					ERROR("%s() failed, addr=0x%lx ret=0x%lx\n",
						"host_realm_destroy_undelegate_range",
						map_addr, ret);
					return REALM_ERROR;
				}
			}
			break;
		case RMI_UNASSIGNED:
			break;
		case RMI_TABLE:
			ret = host_realm_tear_down_rtt_range(realm, level + 1L,
							     map_addr,
							     end_addr);
			if (ret != RMI_SUCCESS) {
				ERROR("%s() failed, map_addr=0x%lx ret=0x%lx\n",
					"host_realm_tear_down_rtt_range",
					map_addr, ret);
				return REALM_ERROR;
			}

			ret = host_realm_destroy_free_rtt(realm, map_addr,
							  level + 1L,
							  rtt_out_addr);
			if (ret != RMI_SUCCESS) {
				ERROR("%s() failed, map_addr=0x%lx ret=0x%lx\n",
					"host_realm_destroy_free_rtt",
					map_addr, ret);
				return REALM_ERROR;
			}

			/* RTT_AUX_DESTROY */
			if (!realm->rtt_s2ap_enc_indirect) {
				ret = host_realm_destroy_free_aux_rtt(realm, map_addr,
						level);

				if (ret != RMI_SUCCESS) {
					ERROR("%s() failed, map_addr=0x%lx ret=0x%lx\n",
						"host_realm_destroy_free_aux_rtt",
						map_addr, ret);
					return REALM_ERROR;
				}
			}
			break;
		default:
			return REALM_ERROR;
		}
	}

	return REALM_SUCCESS;
}

u_register_t host_rmi_granule_delegate(u_register_t addr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_GRANULE_DELEGATE, addr},
				2U).ret0;
}

u_register_t host_rmi_granule_undelegate(u_register_t addr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_GRANULE_UNDELEGATE, addr},
				2U).ret0;
}

u_register_t host_rmi_version(u_register_t requested_ver)
{
	smc_ret_values ret;

	ret = host_rmi_handler(&(smc_args) {SMC_RMI_VERSION, requested_ver}, 2U);
	if (ret.ret0 == (u_register_t)SMC_UNKNOWN) {
		return SMC_UNKNOWN;
	}
	/* Return lower version. */
	return ret.ret1;
}

u_register_t host_rmi_rtt_set_s2ap(u_register_t rd,
				u_register_t rec,
				u_register_t base,
				u_register_t top,
				u_register_t *out_top,
				u_register_t *rtt_tree)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_RTT_SET_S2AP,
				rd, rec, base, top,
				(u_register_t)&rets}, 6U);
	*out_top = rets.ret1;
	*rtt_tree = rets.ret2;
	return rets.ret0;
}

u_register_t host_realm_create(struct realm *realm)
{
	struct rmi_realm_params *params;
	u_register_t ret;
	unsigned int count, rtt_page_count = 0U;

	realm->par_size = REALM_MAX_LOAD_IMG_SIZE;

	realm->state = REALM_STATE_NULL;

	/* Initialize  Host NS heap memory to be used in Realm creation*/
	spin_lock(&pool_lock);
	count = pool_counter;
	pool_counter++;
	spin_unlock(&pool_lock);

	if (count == 0U) {
		if (page_pool_init(PAGE_POOL_BASE, PAGE_POOL_MAX_SIZE)
			!= HEAP_INIT_SUCCESS) {
			ERROR("%s() failed\n", "page_pool_init");
			return REALM_ERROR;
		}
	}

	assert(realm->num_aux_planes <= MAX_AUX_PLANE_COUNT);

	/*
	 * Allocate memory for PAR - Realm image for each Plane.
	 * Granule delegation
	 * of PAR will be performed during rtt creation.
	 * realm->par_size is size if single Plane image.
	 * Same image is copied for each plane.
	 * Offset for Plane N will be realm->par_base + (N*realm->par_size)
	 */
	realm->par_base = (u_register_t)page_alloc((realm->par_size) *
			(realm->num_aux_planes + 1U));

	if (realm->par_base == HEAP_NULL_PTR) {
		ERROR("page_alloc failed, base=0x%lx, size=0x%lx\n",
			  realm->par_base, realm->par_size);
		goto pool_reset;
	}

	INFO("Realm start adr=0x%lx\n", realm->par_base);

	/* Allocate memory for params */
	params = (struct rmi_realm_params *)page_alloc(PAGE_SIZE);
	if (params == NULL) {
		ERROR("Failed to allocate memory for params\n");
		goto pool_reset;
	}

	memset(params, 0, PAGE_SIZE);

	/* Allocate and delegate RD */
	realm->rd = (u_register_t)page_alloc(PAGE_SIZE);
	if (realm->rd == HEAP_NULL_PTR) {
		ERROR("Failed to allocate memory for rd\n");
		goto err_free_par;
	} else {
		ret = host_rmi_granule_delegate(realm->rd);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, rd=0x%lx ret=0x%lx\n",
				"host_rmi_granule_delegate", realm->rd, ret);
			goto err_free_rd;
		}
	}

	/* Allocate and delegate RTT */
	if (realm->rtt_tree_single) {
		rtt_page_count = 1U;
	} else {
		/* Primary + AUX RTT Tree */
		rtt_page_count = (realm->num_aux_planes + 1U);
	}

	realm->rtt_addr = (u_register_t)page_alloc(rtt_page_count * PAGE_SIZE);

	if (realm->rtt_addr == HEAP_NULL_PTR) {
		ERROR("Failed to allocate memory for rtt_addr\n");
		goto err_undelegate_rd;
	} else {
		for (unsigned int i = 0U; i < rtt_page_count; i++) {
			ret = host_rmi_granule_delegate(realm->rtt_addr + (i * PAGE_SIZE));

			if (ret != RMI_SUCCESS) {
				ERROR("%s() failed, rtt_addr=0x%lx ret=0x%lx\n",
				"host_rmi_granule_delegate", realm->rtt_addr, ret);

				for (unsigned int j = 0U; j < i; j++) {
					host_rmi_granule_undelegate(realm->rtt_addr
							+ (i * PAGE_SIZE));
				}

				goto err_free_rtt;
			}

			if (i > 0U && !realm->rtt_tree_single) {
				realm->aux_rtt_addr[i - 1] = realm->rtt_addr + (i * PAGE_SIZE);
				params->aux_rtt_base[i - 1] = realm->rtt_addr + (i * PAGE_SIZE);
			}
		}
	}

	/* Populate params */
	params->s2sz = EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ,
				realm->rmm_feat_reg0);
	params->num_bps = realm->num_bps;
	params->num_wps = realm->num_wps;

	/* SVE enable and vector length */
	if ((realm->rmm_feat_reg0 & RMI_FEATURE_REGISTER_0_SVE_EN) != 0UL) {
		params->flags0 = RMI_REALM_FLAGS0_SVE;
		params->sve_vl = realm->sve_vl;
	} else {
		params->flags0 = 0UL;
		params->sve_vl = 0U;
	}

	/* PMU enable and number of event counters */
	if ((realm->rmm_feat_reg0 & RMI_FEATURE_REGISTER_0_PMU_EN) != 0UL) {
		params->flags0 |= RMI_REALM_FLAGS0_PMU;
		params->pmu_num_ctrs = realm->pmu_num_ctrs;
	} else {
		params->pmu_num_ctrs = 0U;
	}

	/* LPA2 enable */
	if ((realm->rmm_feat_reg0 & RMI_FEATURE_REGISTER_0_LPA2) != 0UL) {
		params->flags0 |= RMI_REALM_FLAGS0_LPA2;
	}

	/* Enabled RMI FEAT_DA */
	if ((realm->rmm_feat_reg0 & RMI_FEATURE_REGISTER_0_DA_EN) != 0UL) {
		params->flags0 |= RMI_REALM_FLAGS0_DA;
	}

	params->rtt_level_start = realm->start_level;
	params->algorithm = RMI_HASH_SHA_256;
	params->vmid = vmid++;
	params->rtt_base = realm->rtt_addr;
	params->rtt_num_start = 1U;

	if (!realm->rtt_tree_single) {
		params->flags1 |= RMI_REALM_FLAGS1_RTT_TREE_PP;
	}

	if (realm->rtt_s2ap_enc_indirect) {
		params->flags1 |= RMI_REALM_FLAGS1_RTT_S2AP_ENCODING_INDIRECT;
	}

	params->num_aux_planes = realm->num_aux_planes;

	/* Allocate VMID for all planes */
	for (unsigned int i = 0U; i < realm->num_aux_planes; i++) {
		params->aux_vmid[i] = (unsigned short)(vmid++);
		realm->aux_vmid[i] = params->aux_vmid[i];
	}

	/* Create Realm */
	ret = host_rmi_realm_create(realm->rd, (u_register_t)params);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, rd=0x%lx ret=0x%lx\n",
			"host_rmi_realm_create", realm->rd, ret);
		goto err_free_vmid;
	}

	realm->vmid = params->vmid;
	ret = host_rmi_rec_aux_count(realm->rd, &realm->num_aux);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, rd=0x%lx ret=0x%lx\n",
			"host_rmi_rec_aux_count", realm->rd, ret);
		host_rmi_realm_destroy(realm->rd);
		goto err_free_vmid;
	}

	realm->state = REALM_STATE_NEW;

	/* Free params */
	page_free((u_register_t)params);
	return REALM_SUCCESS;

err_free_vmid:
	/* Free VMID */
	for (unsigned int i = 0U; i <= realm->num_aux_planes; i++) {
		vmid--;
	}

err_free_rtt:
	page_free(realm->rtt_addr);

err_undelegate_rd:
	ret = host_rmi_granule_undelegate(realm->rd);
	if (ret != RMI_SUCCESS) {
		WARN("%s() failed, rd=0x%lx ret=0x%lx\n",
			"host_rmi_granule_undelegate", realm->rd, ret);
	}

err_free_rd:
	page_free(realm->rd);

	page_free((u_register_t)params);

	for (unsigned int i = 0U; i < rtt_page_count; i++) {
		ret = host_rmi_granule_undelegate(realm->rtt_addr + (i * PAGE_SIZE));
		if (ret != RMI_SUCCESS) {
			WARN("%s() failed, rtt_addr=0x%lx ret=0x%lx\n",
			"host_rmi_granule_undelegate", realm->rtt_addr + (i * PAGE_SIZE), ret);
		}
	}

err_free_par:
	page_free(realm->par_base);

pool_reset:
	/* Free test resources */
	spin_lock(&pool_lock);
	pool_counter--;
	count = pool_counter;
	spin_unlock(&pool_lock);

	if (count == 0U) {
		INFO("Reset pool\n");
		page_pool_reset();
	}

	return REALM_ERROR;
}

u_register_t host_realm_map_payload_image(struct realm *realm,
					  u_register_t realm_payload_adr)
{
	u_register_t src_pa = realm_payload_adr;
	u_register_t ret;

	/* MAP image regions */
	/* Copy Plane 0-N Images */

	for (unsigned int j = 0U; j <= realm->num_aux_planes; j++) {
		ret = host_realm_delegate_map_protected_data(false, realm,
					realm->par_base + (j * realm->par_size),
					realm->par_size,
					src_pa);

		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed par_base=0x%lx size=0x%lx j=%u ret=0x%lx\n",
			"host_realm_delegate_map_protected_data",
			realm->par_base, realm->par_size, j, ret);
			return REALM_ERROR;
		}
	}

	return REALM_SUCCESS;
}

u_register_t host_realm_init_ipa_state(struct realm *realm, long level,
					u_register_t start, uint64_t end)
{
	u_register_t rd = realm->rd, ret;
	u_register_t top;

	do {
		if (level > RTT_MAX_LEVEL) {
			return REALM_ERROR;
		}

		ret = host_rmi_rtt_init_ripas(rd, start, end, &top);
		if (RMI_RETURN_STATUS(ret) == RMI_ERROR_RTT) {
			int8_t cur_level = RMI_RETURN_INDEX(ret);

			if ((int)cur_level < level) {
				u_register_t cret;

				cret = host_rmi_create_rtt_levels(realm,
								  start,
								  cur_level,
								  level);
				if (cret != RMI_SUCCESS) {
					ERROR("%s() failed, ret=0x%lx line=%u\n",
						"host_rmi_create_rtt_levels",
						cret, __LINE__);
					return REALM_ERROR;
				}

				/* Retry with the RTT levels in place */
				continue;
			}
		}

		/* Retry on the next level */
		level++;

	} while (RMI_RETURN_STATUS(ret) == RMI_ERROR_RTT);

	return ret == RMI_SUCCESS ? RMI_SUCCESS : REALM_ERROR;
}

u_register_t host_realm_map_ns_shared(struct realm *realm,
					u_register_t ns_shared_mem_adr,
					u_register_t ns_shared_mem_size)
{
	u_register_t ret;

	realm->ipa_ns_buffer = ns_shared_mem_adr |
			(1UL << (EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ,
			realm->rmm_feat_reg0) - 1));
	realm->ns_buffer_size = ns_shared_mem_size;

	/* MAP SHARED_NS region */
	for (unsigned int i = 0U; i < ns_shared_mem_size / PAGE_SIZE; i++) {
		ret = host_realm_map_unprotected(realm,
				ns_shared_mem_adr + (i * PAGE_SIZE),
				PAGE_SIZE);

		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, par_base=0x%lx ret=0x%lx\n",
				"host_realm_map_unprotected",
				(ns_shared_mem_adr + (i * PAGE_SIZE)), ret);
			return REALM_ERROR;
		}
	}

	/* AUX MAP NS buffer for all RTTs */
	if (!realm->rtt_tree_single) {
		for (unsigned int j = 0U; j < realm->num_aux_planes; j++) {
			for (unsigned int i = 0U; i < ns_shared_mem_size / PAGE_SIZE; i++) {
				u_register_t fail_index, level_pri, state;

				ret = host_rmi_rtt_aux_map_unprotected(realm->rd,
						realm->ipa_ns_buffer + (i * PAGE_SIZE),
						j + 1, &fail_index, &level_pri, &state);

				if (ret == RMI_SUCCESS) {
					continue;
				} else if (RMI_RETURN_STATUS(ret) == RMI_ERROR_RTT_AUX) {

					/* Create Level and retry */
					int8_t ulevel = RMI_RETURN_INDEX(ret);
					long level = (long)ulevel;

					ret = host_realm_create_rtt_aux_levels(realm,
							realm->ipa_ns_buffer +
							(i * PAGE_SIZE),
							level, 3, j + 1);

					if (ret != RMI_SUCCESS) {
						return REALM_ERROR;
					}

					ret = host_rmi_rtt_aux_map_unprotected(realm->rd,
							realm->ipa_ns_buffer +
							(i * PAGE_SIZE),
							j + 1, &fail_index,
							&level_pri, &state);

					if (ret == RMI_SUCCESS) {
						continue;
					}
				}

				ERROR("%s() failed, par_base=0x%lx ret=0x%lx\n",
						"host_realm_aux_map_unprotected",
						(ns_shared_mem_adr), ret);
				return REALM_ERROR;

			}
		}
	}

	return REALM_SUCCESS;
}

/* Free AUX pages for rec0 to rec_num */
static void host_realm_free_rec_aux(u_register_t
		(*aux_pages)[REC_PARAMS_AUX_GRANULES],
		unsigned int num_aux, unsigned int rec_num)
{
	u_register_t ret;

	assert(rec_num < MAX_REC_COUNT);
	assert(num_aux <= REC_PARAMS_AUX_GRANULES);
	for (unsigned int i = 0U; i <= rec_num; i++) {
		for (unsigned int j = 0U; j < num_aux &&
					aux_pages[i][j] != 0U; j++) {
			ret = host_rmi_granule_undelegate(aux_pages[i][j]);
			if (ret != RMI_SUCCESS) {
				WARN("%s() failed, index=%u,%u ret=0x%lx\n",
				"host_rmi_granule_undelegate", i, j, ret);
			}
			page_free(aux_pages[i][j]);
		}
	}
}

static u_register_t host_realm_alloc_rec_aux(struct realm *realm,
		struct rmi_rec_params *params, u_register_t rec_num)
{
	u_register_t ret;
	unsigned int j;

	assert(rec_num < MAX_REC_COUNT);
	for (j = 0U; j < realm->num_aux; j++) {
		params->aux[j] = (u_register_t)page_alloc(PAGE_SIZE);
		if (params->aux[j] == HEAP_NULL_PTR) {
			ERROR("Failed to allocate memory for aux rec\n");
			return RMI_ERROR_REALM;
		}
		ret = host_rmi_granule_delegate(params->aux[j]);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, index=%u ret=0x%lx\n",
				"host_rmi_granule_delegate", j, ret);
			/*
			 * Free current page,
			 * prev pages freed at host_realm_free_rec_aux
			 */
			page_free(params->aux[j]);
			params->aux[j] = 0UL;
			return RMI_ERROR_REALM;
		}

		/* We need a copy in Realm object for final destruction */
		realm->aux_pages_all_rec[rec_num][j] = params->aux[j];
	}
	return RMI_SUCCESS;
}

u_register_t host_realm_rec_create(struct realm *realm)
{
	struct rmi_rec_params *rec_params;
	u_register_t ret;
	unsigned int i;

	for (i = 0U; i < realm->rec_count; i++) {
		realm->run[i] = 0U;
		realm->rec[i] = 0U;
		realm->mpidr[i] = 0U;
	}
	(void)memset(realm->aux_pages_all_rec, 0x0, sizeof(u_register_t) *
			realm->num_aux*realm->rec_count);

	/* Allocate memory for rec_params */
	rec_params = (struct rmi_rec_params *)page_alloc(PAGE_SIZE);
	if (rec_params == NULL) {
		ERROR("Failed to allocate memory for rec_params\n");
		return REALM_ERROR;
	}

	for (i = 0U; i < realm->rec_count; i++) {
		(void)memset(rec_params, 0x0, PAGE_SIZE);

		/* Allocate memory for run object */
		realm->run[i] = (u_register_t)page_alloc(PAGE_SIZE);
		if (realm->run[i] == HEAP_NULL_PTR) {
			ERROR("Failed to allocate memory for run\n");
			goto err_free_mem;
		}
		(void)memset((void *)realm->run[i], 0x0, PAGE_SIZE);

		/* Allocate and delegate REC */
		realm->rec[i] = (u_register_t)page_alloc(PAGE_SIZE);
		if (realm->rec[i] == HEAP_NULL_PTR) {
			ERROR("Failed to allocate memory for REC\n");
			goto err_free_mem;
		} else {
			ret = host_rmi_granule_delegate(realm->rec[i]);
			if (ret != RMI_SUCCESS) {
				ERROR("%s() failed, rec=0x%lx ret=0x%lx\n",
				"host_rmi_granule_delegate", realm->rd, ret);
				goto err_free_mem;
			}
		}

		/* Delegate the required number of auxiliary Granules  */
		ret = host_realm_alloc_rec_aux(realm, rec_params, i);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, ret=0x%lx\n", "host_realm_alloc_rec_aux",
			ret);
			goto err_free_aux;
		}

		/* Populate rec_params */
		rec_params->pc = realm->par_base;
		rec_params->flags = realm->rec_flag[i];

		/* Convert REC index to RmiRecMpidr type */
		rec_params->mpidr = RMI_REC_MPIDR((u_register_t)i);
		rec_params->num_aux = realm->num_aux;
		realm->mpidr[i] = rec_params->mpidr;

		/* Create REC  */
		ret = host_rmi_rec_create(realm->rd, realm->rec[i],
				(u_register_t)rec_params);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed,index=%u, ret=0x%lx\n",
					"host_rmi_rec_create", i, ret);
			goto err_free_aux;
		}
	}
	/* Free rec_params */
	page_free((u_register_t)rec_params);
	return REALM_SUCCESS;

err_free_aux:
	host_realm_free_rec_aux(realm->aux_pages_all_rec, realm->num_aux, i);

err_free_mem:
	for (unsigned int j = 0U; j <= i ; j++) {
		ret = host_rmi_granule_undelegate(realm->rec[j]);
		if (ret != RMI_SUCCESS) {
			WARN("%s() failed, rec=0x%lx ret=0x%lx\n",
			"host_rmi_granule_undelegate", realm->rec[j], ret);
		}
		page_free(realm->run[j]);
		page_free(realm->rec[j]);
	}
	page_free((u_register_t)rec_params);
	return REALM_ERROR;
}

u_register_t host_realm_activate(struct realm *realm)
{
	u_register_t ret;

	/* Activate Realm  */
	ret = host_rmi_realm_activate(realm->rd);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, ret=0x%lx\n", "host_rmi_realm_activate",
			ret);
		return REALM_ERROR;
	}

	realm->state = REALM_STATE_ACTIVE;

	return REALM_SUCCESS;
}

u_register_t host_realm_destroy(struct realm *realm)
{
	u_register_t ret;
	unsigned int rtt_page_count;
	long rtt_start_level = realm->start_level;

	if (realm->state == REALM_STATE_NULL) {
		return REALM_SUCCESS;
	}

	/* For each REC - Destroy, undelegate and free */
	for (unsigned int i = 0U; i < realm->rec_count; i++) {
		if (realm->rec[i] == 0U) {
			break;
		}

		ret = host_rmi_rec_destroy(realm->rec[i]);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, rec=0x%lx ret=0x%lx\n",
				"host_rmi_rec_destroy", realm->rec[i], ret);
			return REALM_ERROR;
		}

		ret = host_rmi_granule_undelegate(realm->rec[i]);
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, rec=0x%lx ret=0x%lx\n",
				"host_rmi_granule_undelegate", realm->rec[i], ret);
			return REALM_ERROR;
		}

		page_free(realm->rec[i]);

		/* Free run object */
		page_free(realm->run[i]);
	}

	host_realm_free_rec_aux(realm->aux_pages_all_rec,
			realm->num_aux, realm->rec_count - 1U);

	/*
	 * For each data granule - Destroy, undelegate and free
	 * RTTs (level 1U and below) must be destroyed leaf-upwards,
	 * using RMI_DATA_DESTROY, RMI_RTT_DESTROY and RMI_GRANULE_UNDELEGATE
	 * commands.
	 */
	if (host_realm_tear_down_rtt_range(realm, rtt_start_level, 0UL,
				(1UL << (EXTRACT(RMI_FEATURE_REGISTER_0_S2SZ,
				realm->rmm_feat_reg0) - 1UL))) != RMI_SUCCESS) {
		ERROR("host_realm_tear_down_rtt_range() line=%u\n", __LINE__);
		return REALM_ERROR;
	}

	if (realm->shared_mem_created == true) {
		if (host_realm_tear_down_rtt_range(realm, rtt_start_level,
				realm->ipa_ns_buffer,
				(realm->ipa_ns_buffer + realm->ns_buffer_size)) !=
				RMI_SUCCESS) {
			ERROR("host_realm_tear_down_rtt_range() line=%u\n", __LINE__);
			return REALM_ERROR;
		}
		page_free(realm->host_shared_data);
	}

	/*
	 * RD Destroy, undelegate and free
	 * RTT(L0) undelegate and free
	 * PAR free
	 */
	ret = host_rmi_realm_destroy(realm->rd);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, rd=0x%lx ret=0x%lx\n",
			"host_rmi_realm_destroy", realm->rd, ret);
		return REALM_ERROR;
	}

	ret = host_rmi_granule_undelegate(realm->rd);
	if (ret != RMI_SUCCESS) {
		ERROR("%s() failed, rd=0x%lx ret=0x%lx\n",
			"host_rmi_granule_undelegate", realm->rd, ret);
		return REALM_ERROR;
	}

	if (realm->rtt_tree_single) {
		rtt_page_count = 1U;
	} else {
		rtt_page_count = realm->num_aux_planes + 1U;
	}

	for (unsigned int i = 0U; i < rtt_page_count; i++) {
		ret = host_rmi_granule_undelegate(realm->rtt_addr + (i * PAGE_SIZE));
		if (ret != RMI_SUCCESS) {
			ERROR("%s() failed, rtt_addr=0x%lx ret=0x%lx\n",
			"host_rmi_granule_undelegate", realm->rtt_addr + (i * PAGE_SIZE), ret);
			return REALM_ERROR;
		}
	}

	/* Free VMID */
	for (unsigned int i = 0U; i <= realm->num_aux_planes; i++) {
		vmid--;
	}

	page_free(realm->rd);
	page_free(realm->rtt_addr);
	page_free(realm->par_base);


	/* Free test resources */
	spin_lock(&pool_lock);
	pool_counter--;
	spin_unlock(&pool_lock);

	return REALM_SUCCESS;
}

unsigned int host_realm_find_rec_by_mpidr(unsigned int mpidr, struct realm *realm)
{
	for (unsigned int i = 0U; i < MAX_REC_COUNT; i++) {
		if (realm->run[i] != 0U && realm->mpidr[i] == mpidr) {
			return i;
		}
	}
	return MAX_REC_COUNT;
}

/* Check if adr is in range of any of the Plane Images */
static bool is_adr_in_par(struct realm *realm, u_register_t addr)
{
	if ((addr >= realm->par_base) && (addr <
			(realm->par_base + (realm->par_size * (realm->num_aux_planes + 1U))))) {
		return true;
	}
	return false;
}

/* Handle Plane permission falut, return true to return to realm, false to host */
static bool host_realm_handle_perm_fault(struct realm *realm, struct rmi_rec_run *run)
{
	/*
	 * If exception is not in primary rtt
	 * Map Adr in failing Aux RTT and re-enter rec
	 * Validate faulting adr is in Realm payload area
	 * Note - PlaneN uses Primary RTT tree index 0
	 */

	u_register_t fail_index;
	u_register_t level_pri;
	u_register_t state;
	u_register_t ripas;
	u_register_t ret;

	VERBOSE("host aux_map 0x%lx rtt 0x%lx\n",
			run->exit.hpfar, run->exit.rtt_tree);

	ret = host_realm_aux_map_protected_data(realm,
			run->exit.hpfar << 8U,
			PAGE_SIZE,
			run->exit.rtt_tree,
			&fail_index, &level_pri, &state, &ripas);

	if (ret != REALM_SUCCESS) {
		ERROR("host_realm_aux_map_protected_data failed\n");
		return false;
	}

	/* re-enter realm */
	return true;
}

/* Handle RSI_MEM_SET_PERM_INDEX by P0, return true to return to realm, false to return to host */
static bool host_realm_handle_s2ap_change(struct realm *realm, struct rmi_rec_run *run,
		u_register_t rec_num)
{


	u_register_t new_base = run->exit.s2ap_base;
	u_register_t top = run->exit.s2ap_top;
	u_register_t rtt_tree, ret;
	bool ret1 = true;

	while (new_base != top) {
		ret = host_rmi_rtt_set_s2ap(realm->rd,
				    realm->rec[rec_num],
				    new_base,
				    top, &new_base,
				    &rtt_tree);

		if (RMI_RETURN_STATUS(ret) == RMI_ERROR_RTT_AUX) {

			int8_t ulevel = RMI_RETURN_INDEX(ret);
			long level = (long)ulevel;

			assert(rtt_tree != PRIMARY_RTT_INDEX);

			/* create missing aux rtt level */
			VERBOSE("set s2ap fail missing aux rtt=%lx 0x%lx 0x%lx\n",
					rtt_tree, new_base, top);

			ret = host_realm_create_rtt_aux_levels(realm, new_base,
						level,
						level + 1, rtt_tree);
			if (ret != RMI_SUCCESS) {
				INFO("host_rmi_create_rtt_aux_levels \
						failed 0x%lx\n", new_base);
				ret1 = false;
				break;
			}

			/* Retry RMI_RTT_SET_S2AP */
			continue;
		} else if (RMI_RETURN_STATUS(ret) == RMI_ERROR_RTT) {

			int8_t ulevel = RMI_RETURN_INDEX(ret);
			long level = (long)ulevel;

			assert(rtt_tree == PRIMARY_RTT_INDEX);
			INFO("set s2ap failed missing rtt=0x%lx 0x%lx 0x%lx\n\n",
						rtt_tree, new_base, top);

			/* create missing rtt level */
			ret = host_rmi_create_rtt_levels(realm, new_base,
							level,
							level + 1);

			if (ret != RMI_SUCCESS) {
				INFO("host_rmi_create_rtt_levels \
							failed 0x%lx\n", new_base);
				ret1 = false;
				break;
			}

			/* Retry RMI_RTT_SET_S2AP */
			continue;
		} else if (RMI_RETURN_STATUS(ret) != RMI_SUCCESS) {
			INFO("host set s2ap failed ret=0x%lx\n",
					RMI_RETURN_STATUS(ret));
			ret1 = false;
			break;
		}
	}

	/* re-enter realm */
	return ret1;
}


u_register_t host_realm_rec_enter(struct realm *realm,
				  u_register_t *exit_reason,
				  unsigned int *host_call_result,
				  unsigned int rec_num)
{
	struct rmi_rec_run *run;
	u_register_t ret;
	bool re_enter_rec;

	if (rec_num >= realm->rec_count) {
		return RMI_ERROR_INPUT;
	}

	run = (struct rmi_rec_run *)realm->run[rec_num];
	realm->host_mpidr[rec_num] = read_mpidr_el1();
	do {
		re_enter_rec = false;
		ret = host_rmi_handler(&(smc_args) {SMC_RMI_REC_ENTER,
					realm->rec[rec_num], realm->run[rec_num]}, 3U).ret0;
		VERBOSE("%s() ret=%lu run->exit.exit_reason=%lu "
			"run->exit.esr=0x%lx EC_BITS=%u ISS_DFSC_MASK=0x%lx\n",
			__func__, ret, run->exit.exit_reason, run->exit.esr,
			((EC_BITS(run->exit.esr) == EC_DABORT_CUR_EL)),
			(ISS_BITS(run->exit.esr) & ISS_DFSC_MASK));

		/* If a data abort because of a GPF */
		if (EC_BITS(run->exit.esr) == EC_DABORT_CUR_EL) {
			ERROR("EC_BITS(run->exit.esr) == EC_DABORT_CUR_EL\n");
			if ((ISS_BITS(run->exit.esr) & ISS_DFSC_MASK) ==
				DFSC_GPF_DABORT) {
				ERROR("DFSC_GPF_DABORT\n");
			}
		}

		if ((run->exit.exit_reason == RMI_EXIT_SYNC) &&
		     is_adr_in_par(realm, (run->exit.hpfar << 8U)) &&
		     (((((run->exit.esr & ISS_FSC_MASK) >= FSC_L0_TRANS_FAULT) &&
		     ((run->exit.esr & ISS_FSC_MASK) <= FSC_L3_TRANS_FAULT)) ||
		     ((run->exit.esr & ISS_FSC_MASK) == FSC_L_MINUS1_TRANS_FAULT))) &&
		     !realm->rtt_s2ap_enc_indirect &&
		     (realm->num_aux_planes > 0U)) {

			re_enter_rec = host_realm_handle_perm_fault(realm, run);
		}

		/*
		 * P0 issued RSI_MEM_SET_PERM_INDEX call
		 * Validate base is in range of realm payload area
		 */
		if ((run->exit.exit_reason == RMI_EXIT_S2AP_CHANGE) &&
		    is_adr_in_par(realm, run->exit.s2ap_base) &&
		    (realm->num_aux_planes > 0U)) {

			re_enter_rec = host_realm_handle_s2ap_change(realm, run, rec_num);
		}

		if (ret != RMI_SUCCESS) {
			return ret;
		}

		if (run->exit.exit_reason == RMI_EXIT_HOST_CALL) {
			switch (run->exit.imm) {
			case HOST_CALL_GET_SHARED_BUFF_CMD:
				run->entry.gprs[0] = realm->ipa_ns_buffer;
				re_enter_rec = true;
				break;
			case HOST_CALL_EXIT_PRINT_CMD:
				realm_print_handler(realm, run->exit.gprs[0],
						REC_IDX(run->exit.gprs[1]));
				re_enter_rec = true;
				break;
			case HOST_CALL_EXIT_SUCCESS_CMD:
				*host_call_result = TEST_RESULT_SUCCESS;
				break;
			case HOST_CALL_EXIT_FAILED_CMD:
				*host_call_result = TEST_RESULT_FAIL;
			default:
				break;
			}
		}

		if (run->exit.exit_reason == RMI_EXIT_VDEV_REQUEST) {
			host_do_vdev_complete(realm->rec[rec_num],
					      run->exit.vdev_id);
			re_enter_rec = true;
		}

		if (run->exit.exit_reason == RMI_EXIT_VDEV_COMM) {
			host_do_vdev_communicate(run->exit.vdev,
						 run->exit.vdev_action);
			re_enter_rec = true;
		}
	} while (re_enter_rec);

	*exit_reason = run->exit.exit_reason;
	return ret;
}

u_register_t host_rmi_pdev_aux_count(u_register_t pdev_ptr, u_register_t *count)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_PDEV_AUX_COUNT, pdev_ptr},
				2U);
	*count = rets.ret1;
	return rets.ret0;
}

u_register_t host_rmi_pdev_create(u_register_t pdev_ptr, u_register_t params_ptr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_PDEV_CREATE, pdev_ptr,
					     params_ptr}, 3U).ret0;
}

u_register_t host_rmi_pdev_get_state(u_register_t pdev_ptr, u_register_t *state)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_PDEV_GET_STATE, pdev_ptr},
				2U);
	*state = rets.ret1;
	return rets.ret0;
}

u_register_t host_rmi_pdev_communicate(u_register_t pdev_ptr,
				       u_register_t data_ptr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_PDEV_COMMUNICATE, pdev_ptr,
					     data_ptr}, 3U).ret0;
}

u_register_t host_rmi_pdev_set_pubkey(u_register_t pdev_ptr,
				      u_register_t pubkey_params_ptr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_PDEV_SET_PUBKEY, pdev_ptr,
					     pubkey_params_ptr}, 3U).ret0;
}

u_register_t host_rmi_pdev_stop(u_register_t pdev_ptr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_PDEV_STOP, pdev_ptr},
				2U).ret0;
}

u_register_t host_rmi_pdev_destroy(u_register_t pdev_ptr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_PDEV_DESTROY, pdev_ptr},
				2U).ret0;
}

u_register_t host_rmi_dev_mem_map(u_register_t rd,
				  u_register_t map_addr,
				  u_register_t level,
				  u_register_t dev_mem_addr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_DEV_MEM_MAP, rd,
					map_addr, level, dev_mem_addr}, 5U).ret0;
}

u_register_t host_rmi_dev_mem_unmap(u_register_t rd,
				    u_register_t map_addr,
				    u_register_t level,
				    u_register_t *pa,
				    u_register_t *top)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_DEV_MEM_UNMAP, rd, map_addr,
				level, (u_register_t)&rets}, 5U);

	*pa = rets.ret1;
	*top = rets.ret2;
	return rets.ret0;
}

u_register_t host_rmi_vdev_create(u_register_t rd_ptr, u_register_t pdev_ptr,
				  u_register_t vdev_ptr,
				  u_register_t params_ptr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_VDEV_CREATE, rd_ptr,
			pdev_ptr, vdev_ptr, params_ptr}, 5U).ret0;
}

u_register_t host_rmi_vdev_complete(u_register_t rec_ptr, u_register_t vdev_ptr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_VDEV_COMPLETE, rec_ptr,
			vdev_ptr}, 3U).ret0;
}

u_register_t host_rmi_vdev_communicate(u_register_t pdev_ptr,
				       u_register_t vdev_ptr,
				       u_register_t dev_comm_data_ptr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_VDEV_COMMUNICATE, pdev_ptr,
			vdev_ptr, dev_comm_data_ptr}, 4U).ret0;
}

u_register_t host_rmi_vdev_get_state(u_register_t vdev_ptr, u_register_t *state)
{
	smc_ret_values rets;

	rets = host_rmi_handler(&(smc_args) {SMC_RMI_VDEV_GET_STATE, vdev_ptr},
				2U);
	*state = rets.ret1;
	return rets.ret0;
}

u_register_t host_rmi_vdev_abort(u_register_t vdev_ptr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_VDEV_ABORT, vdev_ptr},
				2U).ret0;
}

u_register_t host_rmi_vdev_stop(u_register_t vdev_ptr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_VDEV_STOP, vdev_ptr},
				2U).ret0;
}

u_register_t host_rmi_vdev_destroy(u_register_t rd_ptr, u_register_t pdev_ptr,
				   u_register_t vdev_ptr)
{
	return host_rmi_handler(&(smc_args) {SMC_RMI_VDEV_DESTROY, rd_ptr,
			pdev_ptr, vdev_ptr}, 4U).ret0;
}
