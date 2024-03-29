/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REALM_TESTS_H
#define REALM_TESTS_H

bool test_pmuv3_cycle_works_realm(void);
bool test_pmuv3_counter(void);
bool test_pmuv3_event_works_realm(void);
bool test_pmuv3_rmm_preserves(void);
bool test_pmuv3_overflow_interrupt(void);
bool test_realm_pauth_set_cmd(void);
bool test_realm_pauth_check_cmd(void);
bool test_realm_pauth_fault(void);
bool test_realm_sve_rdvl(void);
bool test_realm_sve_read_id_registers(void);
bool test_realm_sve_probe_vl(void);
bool test_realm_sve_ops(void);
bool test_realm_sve_fill_regs(void);
bool test_realm_sve_cmp_regs(void);
bool test_realm_sve_undef_abort(void);
bool test_realm_multiple_rec_psci_denied_cmd(void);
bool test_realm_multiple_rec_multiple_cpu_cmd(void);
bool test_realm_sme_read_id_registers(void);
bool test_realm_sme_undef_abort(void);

#endif /* REALM_TESTS_H */

