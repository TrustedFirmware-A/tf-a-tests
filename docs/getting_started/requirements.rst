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

TFTF includes several dependencies managed as Git submodules. You can find the
full list of these dependencies in the `.gitmodules`_ file.

Initial Clone with Submodules
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you're cloning the repository for the first time, run the following commands
to initialize and fetch all submodules:

.. code-block:: bash

   git clone --recurse-submodules "https://git.trustedfirmware.org/TF-A/tf-a-tests"

This ensures all submodules are correctly checked out.

Updating Submodules
^^^^^^^^^^^^^^^^^^^

If the project updates the reference to a submodule (e.g., points to a new
commit of ``libtl``), you can update your local copy by running:

.. code-block:: bash

   git pull --rebase --no-ff
   git submodule update --init --recursive

To fetch the latest commits from all submodules, you can use:

.. code-block:: bash

   git submodule update --remote

Toolchain
---------

Install the required packages to build TF-A Tests with the following command:

::

    sudo apt-get install device-tree-compiler build-essential git python3

Note that at least Python 3.8 is required.

Download and install the GNU cross-toolchain from Arm. The TF-A Tests have
been tested with version 14.3.Rel1 (GCC 14.3):

-  `GCC cross-toolchain`_

In addition, the following optional packages and tools may be needed:

-   For debugging, Arm `Development Studio (Arm-DS)`_.

.. _GCC cross-toolchain: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/downloads
.. _Development Studio (Arm-DS): https://developer.arm.com/Tools%20and%20Software/Arm%20Development%20Studio
.. _.gitmodules: https://review.trustedfirmware.org/plugins/gitiles/TF-A/tf-a-tests/+/refs/heads/master/.gitmodules

--------------

*Copyright (c) 2019-2025, Arm Limited. All rights reserved.*
