..
  Copyright (c) 2023-2026, Advanced Micro Devices, Inc. All rights reserved. !

  SPDX-License-Identifier: BSD-3-Clause !


AMD Versal Gen 2
================

- Logs are available only on console and not saved in memory(No NVM support).
- Versal Gen 2 Platform uses TTC Timer


Platform Variants
-----------------

The Versal2 platform supports multiple topology configurations through the
``VERSAL2_VARIANT`` build option. This allows for scalable configuration of
cluster and core counts. Each value encodes the topology as
``(cluster count * 10 + cores per cluster)`` (e.g. 42: four clusters, two
cores each; 14: one cluster, four cores).

Available Variants:

- ``42`` (default): 4 clusters with 2 cores per cluster.
- ``14``: 1 cluster with 4 cores.
  Chip name: Versal Prime Series Gen 2 2vm3654


Console Selection
-----------------

The Versal2 platform supports selecting the UART console through the
``VERSAL2_CONSOLE`` build option. The base address for the selected console
is resolved in the platform header.

Available Options:

- pl011_1 (Default): Uses UART1 (``PL011_UART1_BASE``)
- pl011 or pl011_0: Uses UART0 (``PL011_UART0_BASE``)

Build Command
-------------
For individual tests/test suite:

.. code-block:: shell

        make CROSS_COMPILE=aarch64-none-elf- PLAT=versal2 TESTS=<required tests> tftf

For Versal2 Specific tests (includes AMD-Xilinx Tests cases + Standard Test Suite)

.. code-block:: shell

        make CROSS_COMPILE=aarch64-none-elf- PLAT=versal2 TESTS=versal tftf

To build with a specific variant (alternate topology):

.. code-block:: shell

        make CROSS_COMPILE=aarch64-none-elf- PLAT=versal2 VERSAL2_VARIANT=14 TESTS=versal tftf

To build with UART0 as the console:

.. code-block:: shell

        make CROSS_COMPILE=aarch64-none-elf- PLAT=versal2 VERSAL2_CONSOLE=pl011_0 TESTS=versal tftf
