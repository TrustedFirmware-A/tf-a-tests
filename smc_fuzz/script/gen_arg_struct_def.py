#
# Copyright (c) 2025 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

import re

def gen_arg_struct_def(asdname,argfieldname,arglst,argnumfield):
	asdfile = open(asdname, "w")
	hline = "/*\n"
	hline += " * Copyright (c) 2024, Arm Limited. All rights reserved.\n"
	hline += " *\n"
	hline += " * SPDX-License-Identifier: BSD-3-Clause\n"
	hline += " */\n"
	hline += "\n"
	hline += "#ifndef ARG_STRUCT_DEF_H\n"
	hline += "#define ARG_STRUCT_DEF_H\n"
	hline += "\n"
	asdfile.write(hline)
	smccount = 0
	argcount = 0
	featset = {}
	hline = ""
	for sn in argfieldname:
		featdes = re.search(r'^([A-Z]+)_.+',sn)
		if featdes:
			if featdes.group(1) not in featset:
				hline += "#define "
				hline += featdes.group(1) + "_INCLUDE" + " 1\n"
				featset[featdes.group(1)] = 1
	hline += "\n"
	asdfile.write(hline)
	hline = ""
	for sn in argfieldname:
		hline = "#define "
		hline += sn
		hline += " "
		hline += str(smccount)
		hline += "\n"
		asdfile.write(hline)
		smccount = smccount + 1
	smccount = smccount - 1
	hline = "#define MAX_SMC_CALLS "
	hline += str(smccount)
	hline += "\n"
	asdfile.write(hline)
	asdfile.write("\n")
	for sn in arglst:
		for an in arglst[sn]:
			hline = "#define "
			hline += sn
			hline += "_ARG" + str(an) + " " + str(argcount)
			hline += "\n"
			asdfile.write(hline)
			argcount = argcount + 1
	argcount = argcount - 1
	hline = "#define MAX_ARG_LENGTH "
	hline += str(argcount)
	hline += "\n\n"
	asdfile.write(hline)
	for sn in argnumfield:
		for ag in argnumfield[sn]:
			fieldcount = 0
			for fn in argnumfield[sn][ag]:
				hline = "#define " + sn + "_ARG" + str(argnumfield[sn][ag][fn]) + "_" + fn.upper() + "_CNT " + str(fieldcount)
				hline += "\n"
				asdfile.write(hline)
				fieldcount = fieldcount + 1
	fieldcount = 0
	hline = "\n\n"
	asdfile.write(hline)
	for sn in argnumfield:
		for ag in argnumfield[sn]:
			for fn in argnumfield[sn][ag]:
				hline = "#define " + sn + "_ARG" + str(argnumfield[sn][ag][fn]) + "_" + fn.upper() + " " +  str(fieldcount)
				hline += "\n"
				asdfile.write(hline)
				fieldcount = fieldcount + 1
	hline = "\n#endif /* ARG_STRUCT_DEF_H */\n"
	asdfile.write(hline)

	asdfile.close()
