/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>

#if __aarch64__

    .globl read128_par_el1
    .globl read128_ttbr0_el1
    .globl read128_ttbr1_el1
    .globl read128_ttbr0_el2
    .globl read128_ttbr1_el2
    .globl read128_vttbr_el2
    .globl read128_rcwmask_el1
    .globl read128_rcwsmask_el1
    .globl write128_par_el1
    .globl write128_ttbr0_el1
    .globl write128_ttbr1_el1
    .globl write128_ttbr0_el2
    .globl write128_ttbr1_el2
    .globl write128_vttbr_el2
    .globl write128_rcwmask_el1
    .globl write128_rcwsmask_el1

func read128_par_el1 /* MRRS x0, x1, S3_0_C7_C4_0 */
    .inst 0xD5787400
    ret
endfunc read128_par_el1

func read128_ttbr0_el1 /* MRRS x0, x1, S3_0_C2_C0_0 */
    .inst 0xD5782000
    ret
endfunc read128_ttbr0_el1

func read128_ttbr1_el1 /* MRRS x0, x1, S3_0_C2_C0_1 */
    .inst 0xD5782020
    ret
endfunc read128_ttbr1_el1

func read128_ttbr0_el2 /* MRRS x0, x1, S3_4_C2_C0_0 */
    .inst 0xD57C2000
    ret
endfunc read128_ttbr0_el2

func read128_ttbr1_el2 /* MRRS x0, x1, S3_4_C2_C0_1 */
    .inst 0xD57C2020
    ret
endfunc read128_ttbr1_el2

func read128_vttbr_el2 /* MRRS x0, x1, S3_4_C2_C1_0 */
    .inst 0xD57C2100
    ret
endfunc read128_vttbr_el2

func read128_rcwmask_el1 /* MRRS x0, x1, S3_0_C13_C0_6 */
    .inst 0xD578D0C0
    ret
endfunc read128_rcwmask_el1

func read128_rcwsmask_el1 /* MRRS x0, x1, S3_0_C13_C0_3 */
    .inst 0xD578D060
    ret
endfunc read128_rcwsmask_el1

func write128_par_el1 /* MSRR S3_0_C7_C4_0, x0, x1 */
    .inst 0xD5587400
    ret
endfunc write128_par_el1

func write128_ttbr0_el1 /* MSRR S3_0_C2_C0_0, x0, x1 */
    .inst 0xD5582000
    ret
endfunc write128_ttbr0_el1

func write128_ttbr1_el1 /* MSRR S3_0_C2_C0_1, x0, x1 */
    .inst 0xD5582020
    ret
endfunc write128_ttbr1_el1

func write128_ttbr0_el2 /* MSRR S3_4_C2_C0_0, x0, x1 */
    .inst 0xD55C2000
    ret
endfunc write128_ttbr0_el2

func write128_ttbr1_el2 /* MSRR S3_4_C2_C0_1, x0, x1 */
    .inst 0xD55C2020
    ret
endfunc write128_ttbr1_el2

func write128_vttbr_el2 /* MSRR S3_4_C2_C1_0, x0, x1 */
    .inst 0xD55C2100
    ret
endfunc write128_vttbr_el2

func write128_rcwmask_el1 /* MSRR S3_0_C13_C0_6, x0, x1 */
    .inst 0xD558D0C0
    ret
endfunc write128_rcwmask_el1

func write128_rcwsmask_el1 /* MSRR S3_0_C13_C0_3, x0, x1 */
    .inst 0xD558D060
    ret
endfunc write128_rcwsmask_el1

#endif /* __aarch64__ */
