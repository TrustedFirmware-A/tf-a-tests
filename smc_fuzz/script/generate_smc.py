# !/usr/bin/env python
#
# Copyright (c) 2025 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#


#python3 generate_smc.py -s smclist
#This script generated C files after the read of the SMC description/list file
import argparse
import readsmclist
import gen_arg_struct_def
import gen_field_specification

parser = argparse.ArgumentParser(
		prog='generate_smc.py',
		description='Generates SMC code to add to fuzzer library',
		epilog='one argument input')

parser.add_argument('-s', '--smclist',help="SMC list file .")

args = parser.parse_args()

print("starting generate SMC")

seq = 0

readsmclist.readsmclist(args.smclist,seq)

arglst = readsmclist.arglst
argnumfield = readsmclist.argnumfield
argfieldname = readsmclist.argfieldname
argstartbit = readsmclist.argstartbit
argendbit = readsmclist.argendbit
argdefval = readsmclist.argdefval
smcname = readsmclist.smcname
argnum = readsmclist.argnum
argname = readsmclist.argname

gen_arg_struct_def.gen_arg_struct_def("./include/arg_struct_def.h",argfieldname,arglst,argnumfield)

gen_field_specification.gen_field_specification("./include/field_specification.h",
argfieldname,argendbit,argstartbit,argdefval,argnumfield)

