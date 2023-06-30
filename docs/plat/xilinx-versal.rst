..
  Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved. !

  SPDX-License-Identifier: BSD-3-Clause !


Xilinx Versal
=============

- The TF-A Tests on Xilinx Versal platfrom runs from DDR.
- Logs are available only on console and not saved in memory(No NVM support).
- Versal Platform uses TTC Timer


Build Command
-------------
For individual tests/test suite:

.. code-block:: shell

        make CROSS_COMPILE=aarch64-none-elf- PLAT=versal TESTS=<required tests> tftf

For Versal Specific tests (includes AMD-Xilinx Tests cases + Standard Test Suite)

.. code-block:: shell

        make CROSS_COMPILE=aarch64-none-elf- PLAT=versal TESTS=versal tftf

Execution on Target
-------------------

- The TF-A Tests uses the memory location of U-boot.
- To package the tftf.elf in BOOT.BIN, the u-boot entry in bootgen.bif needs to be replaced with following

.. code-block:: shell

        the_ROM_image:
        {
                image {
                        { type=bootimage, file=project-spec/hw-description/vpl_gen_fixed.pdi }
                        { type=bootloader, file=plm.elf }
                        { core=psm, file=psmfw.elf }
                }
                image {
                        id = 0x1c000000, name=apu_subsystem
                        { type=raw, load=0x00001000, file=system-default.dtb }
                        { core=a72-0, exception_level=el-3, trustzone, file=bl31.elf }
                        { core=a72-0, file=tftf.elf }
                }
        }

- The BOOT.BIN with TF-A Tests can now be used to run on the target.
- The TF-A Tests will be executed after TF-A and the tests report will be available on the console.
