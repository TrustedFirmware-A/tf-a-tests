Prerequisites & Requirements
============================

This document describes the software and hardware requiremnts for building TF-A
Tests for AArch32 and AArch64 target platforms.

It may be possible to build TF-A Tests with combinations of software and
hardware that are different from those listed below. The software and hardware
described in this document are officially supported.

Build Host
----------

TF-A Tests may be built using a Linux build host machine with a recent Linux
distribution. We have performed tests using Ubuntu 22.04 LTS (64-bit), but other
distributions should also work fine, provided that the tools and libraries
can be installed.

Dependencies
------------

This section lists the dependencies for TF-A-Tests which are added as
as a git submodule.

======================== =====================
        Name             Version
======================== =====================
Mbed TLS                 3.6.4
Transfer List Library    0.0.1
Event Log Library        0.0.1
======================== =====================

Toolchain
---------

Install the required packages to build TF-A Tests with the following command:

::

    sudo apt-get install device-tree-compiler build-essential git python3

Note that at least Python 3.8 is required.

Download and install the GNU cross-toolchain from Arm. The TF-A Tests have
been tested with version 14.2.Rel1 (GCC 14.2):

-  `GCC cross-toolchain`_

In addition, the following optional packages and tools may be needed:

-   For debugging, Arm `Development Studio (Arm-DS)`_.

.. _GCC cross-toolchain: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/downloads
.. _Development Studio (Arm-DS): https://developer.arm.com/Tools%20and%20Software/Arm%20Development%20Studio

--------------

*Copyright (c) 2019-2025, Arm Limited. All rights reserved.*
