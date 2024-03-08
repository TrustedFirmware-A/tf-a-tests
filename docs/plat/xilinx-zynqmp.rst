..
  Copyright (c) 2024, Advanced Micro Devices, Inc. All rights reserved. !
  SPDX-License-Identifier: BSD-3-Clause !

Xilinx ZynqMP
=============

- The TF-A Tests on Xilinx ZynqMP platform runs from DDR.
- Logs are available only on console and not saved in memory(No NVM support).
- ZynqMP Platform uses TTC Timer

Build Command
-------------
For individual tests/test suite:

.. code-block:: shell

        make CROSS_COMPILE=aarch64-none-elf- PLAT=zynqmp TESTS=<required tests> tftf

For Versal NET Specific tests (includes AMD-Xilinx Tests cases + Standard Test Suite)

.. code-block:: shell

        make CROSS_COMPILE=aarch64-none-elf- PLAT=zynqmp TESTS=versal tftf

Execution on Target
-------------------

- The TF-A Tests uses the memory location of U-boot.
- To package the tftf.elf in BOOT.BIN, the u-boot entry in bootgen.bif needs to be replaced with following

.. code-block:: shell

	the_ROM_image:
	{
		[bootloader, destination_cpu=a53-0] zynqmp_fsbl.elf
		[pmufw_image] pmufw.elf
		[destination_device=pl] pre-built/linux/implementation/download.bit
		[destination_cpu=a53-0, exception_level=el-3, trustzone] bl31.elf
		[destination_cpu=a53-0, load=0x00100000] system.dtb
		[destination_cpu=a53-0, exception_level=el-2] tftf.elf
	}

- The BOOT.BIN with TF-A Tests can now be used to run on the target.
- The TF-A Tests will be executed after TF-A and the tests report will be available on the console.
