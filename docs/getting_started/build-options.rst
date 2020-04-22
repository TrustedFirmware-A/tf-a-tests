Build Options Summary
=====================

As far as possible, TF-A Tests dynamically detects the platform hardware
components and available features. There are a few build options to select
specific features where the dynamic detection falls short.

Unless mentioned otherwise, these options are expected to be specified at the
build command line and are not to be modified in any component makefiles.

.. note::
   The build system doesn't track dependencies for build options. Therefore, if
   any of the build options are changed from a previous build, a clean build
   must be performed.

Common (Shared) Build Options
-----------------------------

Most of the build options listed in this section apply to TFTF, the FWU test
images and Cactus, unless otherwise specified. These do not influence the EL3
payload, whose simplistic build system is mostly independent.

-  ``ARCH``: Choose the target build architecture for TF-A Tests. It can take
   either ``aarch64`` or ``aarch32`` as values. By default, it is defined to
   ``aarch64``. Not all test images support this build option.

-  ``ARM_ARCH_MAJOR``: The major version of Arm Architecture to target when
   compiling TF-A Tests. Its value must be numeric, and defaults to 8.

-  ``ARM_ARCH_MINOR``: The minor version of Arm Architecture to target when
   compiling TF-A Tests. Its value must be a numeric, and defaults to 0.

-  ``DEBUG``: Chooses between a debug and a release build. A debug build
   typically embeds assertions checking the validity of some assumptions and its
   output is more verbose. The option can take either 0 (release) or 1 (debug)
   as values. 0 is the default.

-  ``ENABLE_ASSERTIONS``: This option controls whether calls to ``assert()`` are
   compiled out.

   -  For debug builds, this option defaults to 1, and calls to ``assert()`` are
      compiled in.
   -  For release builds, this option defaults to 0 and calls to ``assert()``
      are compiled out.

   This option can be set independently of ``DEBUG``. It can also be used to
   hide any auxiliary code that is only required for the assertion and does not
   fit in the assertion itself.

-  ``LOG_LEVEL``: Chooses the log level, which controls the amount of console log
   output compiled into the build. This should be one of the following:

   ::

       0  (LOG_LEVEL_NONE)
       10 (LOG_LEVEL_ERROR)
       20 (LOG_LEVEL_NOTICE)
       30 (LOG_LEVEL_WARNING)
       40 (LOG_LEVEL_INFO)
       50 (LOG_LEVEL_VERBOSE)

   All log output up to and including the selected log level is compiled into
   the build. The default value is 40 in debug builds and 20 in release builds.

-  ``PLAT``: Choose a platform to build TF-A Tests for. The chosen platform name
   must be a subdirectory of any depth under ``plat/``, and must contain a
   platform makefile named ``platform.mk``. For example, to build TF-A Tests for
   the Arm Juno board, select ``PLAT=juno``.

-  ``V``: Verbose build. If assigned anything other than 0, the build commands
   are printed. Default is 0.

TFTF-specific Build Options
---------------------------

-  ``ENABLE_PAUTH``: Boolean option to enable ARMv8.3 Pointer Authentication
   (``ARMv8.3-PAuth``) support in the Trusted Firmware-A Test Framework itself.
   If enabled, it is needed to use a compiler that supports the option
   ``-mbranch-protection`` (GCC 9 and later). It defaults to 0.

-  ``NEW_TEST_SESSION``: Choose whether a new test session should be started
   every time or whether the framework should determine whether a previous
   session was interrupted and resume it. It can take either 1 (always
   start new session) or 0 (resume session as appropriate). 1 is the default.

-  ``TESTS``: Set of tests to run. Use the following command to list all
   possible sets of tests:

   ::

     make help_tests

   If no set of tests is specified, the standard tests will be selected (see
   ``tftf/tests/tests-standard.xml``).

-  ``USE_NVM``: Used to select the location of test results. It can take either 0
   (RAM) or 1 (non-volatile memory like flash) as test results storage. Default
   value is 0, as writing to the flash significantly slows tests down.

FWU-specific Build Options
--------------------------

-  ``FIRMWARE_UPDATE``: Whether the Firmware Update test images (i.e.
   ``NS_BL1U`` and ``NS_BL2U``) should be built. The default value is 0.  The
   platform makefile is free to override this value if Firmware Update is
   supported on this platform.

--------------

*Copyright (c) 2019, Arm Limited. All rights reserved.*