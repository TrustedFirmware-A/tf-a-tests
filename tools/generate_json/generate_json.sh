#!/bin/bash

#
# Copyright (c) 2020-2022, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Generate a JSON file which will be fed to TF-A as SPM_LAYOUT_FILE to package
# Secure Partitions as part of FIP.
# Note the script will append the partition to the existing layout file.
# If you wish to only generate a layout file with this partition first run
# "make realclean" to remove the existing file.

# $1 = Platform built path
# $2.. = List of Secure Partitions
# Output = $1/sp_layout.json

GENERATED_JSON=$1/sp_layout.json
shift # Shift arguments 1

PARTITION_ALREADY_PRESENT=false

CACTUS_PRESENT=false
IVY_PRESENT=false
IVY_SHIM_PRESENT=false

for target in "$@"; do
	case $target in
		cactus) CACTUS_PRESENT=true ;;
		ivy) IVY_PRESENT=true ;;
		ivy_shim) IVY_SHIM_PRESENT=true ;;
		*) echo "Invalid target $target"; exit 1 ;;
	esac
done

echo -e "{" > "$GENERATED_JSON"

# To demonstrate communication between SP's, two cactus S-EL1 instances used.
# To also test mapping of the RXTX region a third cactus S-EL1 instance is used.
# cactus-primary, cactus-secondary and cactus-tertiary have same binary but
# different partition manifests.
if [ $CACTUS_PRESENT == "true" ]; then
	cat >> "$GENERATED_JSON" << EOF
"cactus-primary" : {
	"image": {
		"file": "cactus.bin",
		"offset":"0x2000"
	},
	"pm": {
		"file": "cactus.dts",
		"offset": "0x1000"
	},
	"physical-load-address": "0x7000000",
	"owner": "SiP"
},

"cactus-secondary" : {
	"image": "cactus.bin",
	"pm": "cactus-secondary.dts",
	"physical-load-address": "0x7100000",
	"owner": "Plat",
	"package": "tl_pkg"
},

"cactus-tertiary" : {
	"image": "cactus.bin",
	"pm": "cactus-tertiary.dts",
	"physical-load-address": "0x7200000",
	"owner": "Plat",
	"package": "tl_pkg",
	"size": "0x300000"
EOF
	PARTITION_ALREADY_PRESENT=true
fi

if [ $IVY_PRESENT == "true" ]; then
	if [ $PARTITION_ALREADY_PRESENT == "true" ]; then
		echo -ne "\t},\n\n" >> "$GENERATED_JSON"
	fi

	cat >> "$GENERATED_JSON" << EOF
"ivy" : {
	"image": "ivy.bin",
	"pm": "ivy-sel0.dts",
	"physical-load-address": "0x7600000",
	"owner": "Plat",
	"package": "tl_pkg"
}
EOF

	PARTITION_ALREADY_PRESENT=true
elif [ $IVY_SHIM_PRESENT == "true" ]; then
	if [ $PARTITION_ALREADY_PRESENT == "true" ]; then
		echo -ne "\t},\n\n" >> "$GENERATED_JSON"
	fi
cat >> "$GENERATED_JSON" << EOF
"ivy" : {
	"image": "ivy.bin",
	"pm": "ivy-sel1.dts",
	"physical-load-address": "0x7600000",
	"owner": "Plat",
	"package":"tl_pkg",
	"size": "0x100000"
}
EOF

	PARTITION_ALREADY_PRESENT=true
else
	echo -ne "\t},\n" >> "$GENERATED_JSON"
fi

echo -e "\n}" >> "$GENERATED_JSON"
