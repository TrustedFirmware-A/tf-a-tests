#
# Copyright (c) 2025 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

import re
import copy
import sys

arglst = {}
argnumfield = {}
argfieldname = {}
argstartbit = {}
argendbit = {}
argdefval = {}
smcid = {}
defval = {}
sdef = {}
smcname = ""
argnum = ""
argname = ""

def readsmclist(smclist,seq):
	smclistfile =  open(smclist, "r")
	smclist_lines = smclistfile.readlines()
	smclistfile.close()
	for sline in smclist_lines:
		svar = 0
		lcon = 0
		sl = sline.strip()
		sinstr = re.search(r'^smc:\s*([a-zA-Z0-9_]+)\s*0x([a-fA-F0-9]+)\s*$',sl)
		if sinstr:
			print("in first")
			smcname = sinstr.group(1)
			arglst[sinstr.group(1)] = []
			argnumfield[sinstr.group(1)] = {}
			argfieldname[sinstr.group(1)] = {}
			argstartbit[sinstr.group(1)] = {}
			argendbit[sinstr.group(1)] = {}
			argdefval[sinstr.group(1)] = {}
			smcid[sinstr.group(2)] = sinstr.group(1)
			svar = 1
			lcon = 1
			argoccupy = {}
			if not seq:
				seq = seq + 1
			else:
				if seq != 2:
					print("1 Error: out of sequence for smc call",end=" ")
					print(smcname)
					sys.exit()
				else:
					seq = 1
		sinstr = re.search(r'^smc:\s*([a-zA-Z0-9_]+)\s+([a-zA-Z0-9_]+)\s*$',sl)
		if sinstr and (svar == 0):
			print("in second")
			smcname = sinstr.group(1)
			arglst[sinstr.group(1)] = []
			argnumfield[sinstr.group(1)] = {}
			argfieldname[sinstr.group(1)] = {}
			argstartbit[sinstr.group(1)] = {}
			argendbit[sinstr.group(1)] = {}
			argdefval[sinstr.group(1)] = {}
			sdef[sinstr.group(1)] = sinstr.group(2)
			smcid[sinstr.group(2)] = sinstr.group(1)
			lcon = 1
			argoccupy = {}
			if not seq:
				seq = seq + 1
			else:
				if seq != 2:
					print("2 Error: out of sequence for smc call",end=" ")
					print(smcname)
					sys.exit()
				else:
					seq = 1
		sinstr = re.search(r'^smc:\s*([a-zA-Z0-9_]+)\s*$',sl)
		if sinstr:
			smcname = sinstr.group(1)
			arglst[sinstr.group(1)] = []
			argnumfield[sinstr.group(1)] = {}
			argfieldname[sinstr.group(1)] = {}
			argstartbit[sinstr.group(1)] = {}
			argendbit[sinstr.group(1)] = {}
			argdefval[sinstr.group(1)] = {}
			lcon = 1
			argoccupy = {}
			if not seq:
				seq = seq + 1
			else:
				if seq != 2:
					print("3 Error: out of sequence for smc call",end=" ")
					print(smcname)
					sys.exit()
				else:
					seq = 1
		sinstr = re.search(r'^arg(\d+)\s*:\s*([a-zA-Z0-9_]+)$',sl)
		if sinstr:
			if sinstr.group(1) in argoccupy:
				print("Error: register already specified for SMC call",end=" ")
				print(smcname,end=" ")
				print("argument",end=" ")
				print(sinstr.group(1))
				sys.exit()
			argnum = sinstr.group(1)
			argname = sinstr.group(2)
			arglst[smcname].append(argnum)
			argnumfield[smcname][sinstr.group(2)] = {}
			argfieldname[smcname][sinstr.group(2)] = []
			argstartbit[smcname][sinstr.group(2)] = {}
			argendbit[smcname][sinstr.group(2)] = {}
			argdefval[smcname][sinstr.group(2)] = {}
			lcon = 1
			argoccupy[argnum] = 1
			fieldoccupy = []
			if seq != 1:
				if seq != 2:
					print("Error: out of sequence for arg(value)",end=" ")
					print("arg",end=" ")
					print(argnum,end=" ")
					print("for argname",end=" ")
					print(argname)
					sys.exit()
				else:
					seq = 2
			else:
				seq = seq + 1

		sinstr = re.search(r'^arg(\d+)\s*=\s*(0x[a-fA-F0-9]+|\d+)$',sl)
		if sinstr:
			if sinstr.group(1) in argoccupy:
				print("Error: register already specified for SMC call",end=" ")
				print(smcname,end=" ")
				print("argument",end=" ")
				print(sinstr.group(1))
				sys.exit()
			srange = sinstr.group(1)
			argrangename = smcname + "_args_"+ sinstr.group(1)
			argnum = srange
			argname = smcname + "_arg_" + argnum
			fieldnameargdef = smcname + "_arg_" + argnum + "_field"
			arglst[smcname].append(argnum)
			argnumfield[smcname][argname] = {}
			argfieldname[smcname][argname] = []
			argstartbit[smcname][argname] = {}
			argendbit[smcname][argname] = {}
			argdefval[smcname][argname] = {}
			argnumfield[smcname][argname][fieldnameargdef] = argnum
			argfieldname[smcname][argname].append(fieldnameargdef)
			argstartbit[smcname][argname][fieldnameargdef] = str(0)
			argendbit[smcname][argname][fieldnameargdef] = str(63)
			argdefval[smcname][argname][fieldnameargdef] = sinstr.group(2)
			lcon = 1
			argoccupy[argnum] = 1
			if seq != 1:
				if seq != 2:
					print("Error: out of sequence for arg(value)",end=" ")
					print("arg",end=" ")
					print(argnum,end=" ")
					print("for argname",end=" ")
					print(argname)
					sys.exit()
			else:
				seq = seq + 1
		sinstr = re.search(r'^arg(\d+)-arg(\d+)\s*=\s*(0x[a-fA-F0-9]+|\d+)$',sl)
		if sinstr:
			srange = int(sinstr.group(1))
			erange = int(sinstr.group(2))
			argrangename = smcname + "_args_"+ sinstr.group(1) + "_" + sinstr.group(2)
			for i in range((erange - srange) + 1):
				if str(srange + i) in argoccupy:
					print("Error: register already specified for SMC call",end=" ")
					print(smcname,end=" ")
					print("argument",end=" ")
					print(str(srange + i))
					sys.exit()
				argnum = srange + i
				argname = smcname + "_arg_" + str(argnum)
				fieldnameargdef = smcname + "_arg_" + str(argnum) + "_field"
				arglst[smcname].append(argnum)
				argnumfield[smcname][argname] = {}
				argfieldname[smcname][argname] = []
				argstartbit[smcname][argname] = {}
				argendbit[smcname][argname] = {}
				argdefval[smcname][argname] = {}
				argnumfield[smcname][argname][fieldnameargdef] = str(argnum)
				argfieldname[smcname][argname].append(fieldnameargdef)
				argstartbit[smcname][argname][fieldnameargdef] = str(0)
				argendbit[smcname][argname][fieldnameargdef] = str(63)
				argdefval[smcname][argname][fieldnameargdef] = sinstr.group(3)
				argoccupy[str(argnum)] = 1
			lcon = 1
			if seq != 1:
				if seq != 2:
					print("Error: out of sequence for arg(value)",end=" ")
					print("arg",end=" ")
					print(argnum,end=" ")
					print("for argname",end=" ")
					print(argname)
					sys.exit()
				else:
					seq = 2
			else:
				seq = seq + 1
		sinstr = re.search(r'^field:([a-zA-Z0-9_]+):\[(\d+),(\d+)\]\s*=\s*(0x[a-fA-F0-9]+|\d+)$',sl)
		if sinstr:
			for fs in fieldoccupy:
				if(((fs[0] <= int(sinstr.group(3))) and (fs[0] >= int(sinstr.group(2)))) or
				((fs[1] <= int(sinstr.group(3))) and (fs[1] >= int(sinstr.group(2))))):
					print("Error: field overlap",end=" ")
					print(smcname,end=" ")
					print(argname,end=" ")
					print(sinstr.group(1),end=" ")
					print(fs[0],end=" ")
					print(fs[1],end=" ")
					print(" with",end=" ")
					print(sinstr.group(2),end=" ")
					print(sinstr.group(3))
					sys.exit()
			argnumfield[smcname][argname][sinstr.group(1)] = argnum
			argfieldname[smcname][argname].append(sinstr.group(1))
			argstartbit[smcname][argname][sinstr.group(1)] = sinstr.group(2)
			argendbit[smcname][argname][sinstr.group(1)] = sinstr.group(3)
			argdefval[smcname][argname][sinstr.group(1)] = sinstr.group(4)
			flist = []
			flist.append(int(sinstr.group(2)))
			flist.append(int(sinstr.group(3)))
			fieldoccupy.append(flist)
			lcon = 1
			if seq != 2:
				print("Error: out of sequence for field")
				sys.exit()
		sinstr = re.search(r'^define ([a-zA-Z0-9_]+)\s*=\s*0x([a-fA-F0-9]+|\d+)$',sl)
		if sinstr:
			defval[sinstr.group(1)] = sinstr.group(2)
			lcon = 1
		if not lcon:
			cline = re.search(r'^#',sl)
			if not cline:
				if sl:
					print("Error: malformed line at",end=" ")
					print(sl)
					sys.exit()
	if(seq != 2):
		print("incorrect ending for smc specification")
		sys.exit()

	for smccal,dval in sdef.items():
		if dval in defval:
			smcid[defval[dval]] = smccal
		else:
			print("Error: cannot find define value",end=" ")
			print(dval)
			sys.exit()
