#
# Copyright (c) 2025 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

def gen_field_specification(fsname,argfieldname,argendbit,argstartbit,argdefval,argnumfield,argres):
	faafile  = open(fsname, "w")
	hline = "struct fuzzer_arg_def {\n"
	hline += "        int regnum;\n"
	hline += "        char smcname[FUZZ_MAX_NAME_SIZE];\n"
	hline += "        char smcargname[FUZZ_MAX_NAME_SIZE];\n"
	hline += "        int bitw;\n"
	hline += "        int bitst;\n"
	hline += "        char bnames[FUZZ_MAX_NAME_SIZE];\n"
	hline += "        uint64_t defval;\n"
	hline += "        uint64_t **contval;\n"
	hline += "        int *contvallen;\n"
	hline += "        int contlen;\n"
	hline += "        int *conttype;\n"
	hline += "        int genvalues;\n"
	hline += "        int reserved;\n"
	hline += "};\n\n"
	hline += "struct fuzzer_arg_arange {\n"
	hline += "        int arg_span[2];\n"
	hline += "};\n"
	faafile.write(hline)
	hline = "struct fuzzer_arg_def fuzzer_arg_array[] = {\n"
	faafile.write(hline)
	ifield = 1
	for sn in argfieldname:
		for an in argfieldname[sn]:
			for fn in argfieldname[sn][an]:
				if not ifield:
					hline = ",\n"
					hline += "{ .bitw = "
				else:
					hline = "{ .bitw = "
				hline += str((int(argendbit[sn][an][fn]) - int(argstartbit[sn][an][fn])) + 1)
				hline += ", .bitst = " + argstartbit[sn][an][fn] + ", .bnames = \""
				hline += fn + "\", .defval = " + argdefval[sn][an][fn] + ", .reserved = "
				hline += str(argres[sn][an][fn]) + ", .regnum = " + argnumfield[sn][an][fn]
				hline += ", .smcname = \"" + sn + "\", .smcargname = \""
				hline += an + "\" }"
				ifield = 0
				faafile.write(hline)
	hline = " };\n\n"
	faafile.write(hline)
	lc = 0
	hline = "struct fuzzer_arg_arange fuzzer_arg_array_lst[] = {" + "\n"
	faafile.write(hline)
	ifield = 1
	for sn in argfieldname:
		for ag in argfieldname[sn]:
			if not ifield:
				hline = ",\n"
				hline += "{ .arg_span = {" + str(lc) + "," + str(len(argfieldname[sn][ag]) - 1 + lc) + "} }"
			else :
				hline = "{ .arg_span = {" + str(lc) + "," + str(len(argfieldname[sn][ag]) - 1 + lc) + "} }"
			ifield = 0
			faafile.write(hline)
			lc = lc + len(argfieldname[sn][ag])
	hline = " };\n\n"
	faafile.write(hline)
	hline = "int fuzzer_arg_array_range[] = {" + "\n"
	faafile.write(hline)
	lc = 0
	ifield = 1
	hline = ""
	for sn in argfieldname:
		if not ifield:
			hline += ","
			hline += str(len(argfieldname[sn]))
		else:
			hline += str(len(argfieldname[sn]))
		lc = lc + 1
		ifield = 0
		if lc == 20:
			hline += "\n"
			lc = 0
	hline += "};\n\n"
	faafile.write(hline)
	hline = "int fuzzer_arg_array_start[] = {" + "\n"
	faafile.write(hline)
	lc = 0
	ifield = 1
	cargs = 0
	hline = ""
	for sn in argfieldname:
		if not ifield:
			hline += ","
			hline += str(cargs)
		else:
			hline += str(cargs)
		cargs = cargs + len(argfieldname[sn])
		lc = lc + 1
		ifield = 0
		if lc == 20:
			hline += "\n"
			lc = 0
	hline += "};\n\n"
	faafile.write(hline)
	hline = "int fuzzer_fieldarg[] = {" + "\n"
	faafile.write(hline)
	blist = 1
	hline = "\t"
	for sn in argnumfield:
		for ag in argnumfield[sn]:
			for fn in argnumfield[sn][ag]:
				if not blist:
					hline += ",\n\t"
				blist = 0
				hline += sn + "_ARG" + str(argnumfield[sn][ag][fn])
				faafile.write(hline)
				hline = ""
	hline = "\n};"
	faafile.write(hline)
	hline = "\n\n"
	faafile.write(hline)
	hline = "int fuzzer_fieldcall[] = {" + "\n"
	faafile.write(hline)
	blist = 1
	hline = "\t"
	for sn in argnumfield:
		for ag in argnumfield[sn]:
			for fn in argnumfield[sn][ag]:
				if not blist:
					hline += ",\n\t"
				blist = 0
				hline += sn
				faafile.write(hline)
				hline = ""
	hline = "\n};"
	faafile.write(hline)
	hline = "\n\n"
	faafile.write(hline)
	hline = "int fuzzer_fieldfld[] = {" + "\n"
	faafile.write(hline)
	blist = 1
	hline = "\t"
	for sn in argnumfield:
		for ag in argnumfield[sn]:
			for fn in argnumfield[sn][ag]:
				if not blist:
					hline += ",\n\t"
				blist = 0
				hline += sn + "_ARG" + str(argnumfield[sn][ag][fn]) + "_" + fn.upper() + "_CNT"
				faafile.write(hline)
				hline = ""
	hline = "\n};"

	faafile.write(hline)

	faafile.close()
