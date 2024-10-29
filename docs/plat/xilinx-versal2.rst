..
  Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved. !

  SPDX-License-Identifier: BSD-3-Clause !


AMD Versal Gen 2
=============

- Logs are available only on console and not saved in memory(No NVM support).
- Versal Gen 2 Platform uses TTC Timer


Build Command
-------------
For individual tests/test suite:

.. code-block:: shell

        make CROSS_COMPILE=aarch64-none-elf- PLAT=versal2 TESTS=<required tests> tftf

For Versal2 Specific tests (includes AMD-Xilinx Tests cases + Standard Test Suite)

.. code-block:: shell

        make CROSS_COMPILE=aarch64-none-elf- PLAT=versal2 TESTS=versal tftf
