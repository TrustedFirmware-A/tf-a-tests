# !/usr/bin/env python
#
# Copyright (c) 2025 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#


#python3 generate_var_coverage.py -sd <smc definition file> -df <SMC data file>
#This script generates variable coverage tables from SMC definition file and output of FVP SMC calls
import re
import copy
import sys
import argparse
import readsmclist
from tabulate import tabulate

parser = argparse.ArgumentParser(
		prog='generate_var_coverage.py',
		description='Creates coverage data from SMC calls in FVP model',
		epilog='two arguments input')

parser.add_argument('-df', '--datafile',help="Data from UART output of the model .")
parser.add_argument('-sd', '--smcdefinition',help="SMC definition file .")

args = parser.parse_args()

print("starting variable coverage")

seq = 0

readsmclist.readsmclist(args.smcdefinition,seq)

arglst = readsmclist.arglst
argnumfield = readsmclist.argnumfield
argfieldname = readsmclist.argfieldname
argstartbit = readsmclist.argstartbit
argendbit = readsmclist.argendbit
argdefval = readsmclist.argdefval
smcname = readsmclist.smcname
argnum = readsmclist.argnum
argname = readsmclist.argname
smcid = readsmclist.smcid

varcovvalues = {}

for sn in argfieldname:
	varcovvalues[sn] = {}
	for an in argfieldname[sn]:
		for fn in argfieldname[sn][an]:
			varcovvalues[sn][fn] = set()

datafile =  open(args.datafile, "r")
data_lines = datafile.readlines()
datafile.close()
for dline in data_lines:
	dl = dline.strip()
	dinstr = re.search(r'^SMC FUZZER CALL fid:([a-fA-F0-9]+)\s+arg1:([a-fA-F0-9]+)\s+arg2:([a-fA-F0-9]+)\s+arg3:([a-fA-F0-9]+)\s+arg4:([a-fA-F0-9]+)\s+arg5:([a-fA-F0-9]+)\s+arg6:([a-fA-F0-9]+)\s+arg7:([a-fA-F0-9]+)$',dl)
	if dinstr:
		if dinstr.group(1) in smcid:
			scall = smcid[dinstr.group(1)]
			for an in argfieldname[scall]:
				for fn in argfieldname[scall][an]:
					if int(argnumfield[scall][an][fn]) < 8:
						argval = dinstr.group(int(argnumfield[scall][an][fn])+1)
						astbit = int(argstartbit[scall][an][fn])
						aenbit = int(argendbit[scall][an][fn])
						vval = hex((int(argval,16) >> astbit) & ((2 ** (aenbit - astbit + 1)) - 1))
						varcovvalues[scall][fn].add(vval)

for sn in varcovvalues:
	print(sn)
	largest = 0
	hdrs = []
	for fn in varcovvalues[sn]:
		if len(varcovvalues[sn][fn]) > largest:
			largest = len(varcovvalues[sn][fn])
	te = [["-" for x in range(len(varcovvalues[sn]))] for x in range(largest)]
	k = 0
	for fn in varcovvalues[sn]:
		i = 0
		hdrs.append(fn)
		for val in varcovvalues[sn][fn]:
			te[i][k] = val
			i = i + 1
		k = k + 1
	print(tabulate(te, headers=hdrs, tablefmt="grid"))
	print()
