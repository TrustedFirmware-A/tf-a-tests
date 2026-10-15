Implementing Tests
==================

This document aims at providing some pointers to help implementing new tests in
the TFTF image.

Test Structure
--------------

A test might be divided into 3 logical parts, detailed in the following
sections.

Prologue
^^^^^^^^

A test has a main entry point function, whose type is:

::

    typedef test_result_t (*test_function_t)(void);

See ``tftf/framework/include/tftf.h``.

Only the primary CPU enters this function, while other CPUs are powered down.

First of all, the test function should check whether this test is applicable to
this platform and environment. Some tests rely on specific hardware features or
firmware capabilities to be present. If these are not available, the test should
be skipped. For example, a multi-core test requires at least 2 CPUs to
run. Macros and functions are provided in ``include/common/test_helpers.h`` to
help test code verify that their requirements are met.

Core
^^^^

This is completely dependent on the purpose of the test. The paragraphs below
just provide some useful, general information.

The primary CPU may power on other CPUs by calling the function
``tftf_cpu_on()``.  It provides an address to which secondary CPUs should jump
to once they have been initialized by the test framework. This address should be
different from the primary CPU test function.

Synchronization primitives are provided in ``include/lib/events.h`` in case CPUs'
execution threads need to be synchronized. Most multi-processing tests will need
some synchronisation points that all/some CPUs need to reach before test
execution may continue.

Any CPU that is involved in a test must return from its test function. Failure
to do so will put the framework in an unrecoverable state, see the
:ref:`Change Log & Release Notes` for details on this and other known
limitations. The return code indicates the test result from the point of view of
this CPU. At the end of the test, individual CPU results are aggregated and the
overall test result is derived from that. A test is considered as passed if all
involved CPUs reported a success status code.

Epilogue
^^^^^^^^

Each test is responsible for releasing any allocated resources and putting the
system back in a clean state when it finishes. Any change to the system
configuration (e.g. MMU setup, GIC configuration, system registers, ...) must be
undone and the original configuration must be restored. This guarantees that the
next test is not affected by the actions of the previous one.

One exception to this rule is that CPUs powered on as part of a test must not be
powered down. As already stated above, as soon as a CPU enters the test, the
framework expects it to return from the test.

.. _implementing_testcase_hooks:

Registering Testcase Hooks
--------------------------

Use setup and cleanup hooks when several tests need the same resource
lifecycle. Define callbacks with the signature described in
:ref:`design_testcase_hooks`, include ``tftf.h``, and register both
callbacks in a source file included in the selected test build.

For example, callbacks named ``setup_resources`` and ``cleanup_resources``
can serve every test in an ``Example tests`` suite:

.. code:: c

   #include <tftf.h>

   TFTF_TESTSUITE_SETUP_HOOK(example_setup_hook,
                            "Example tests", setup_resources);
   TFTF_TESTSUITE_CLEANUP_HOOK(example_cleanup_hook,
                              "Example tests", cleanup_resources);

To apply the lifecycle only to ``Example testcase`` in that suite,
replace the suite registrations with:

.. code:: c

   TFTF_TESTCASE_SETUP_HOOK(example_setup_hook,
                           "Example tests", "Example testcase",
                           setup_resources);
   TFTF_TESTCASE_CLEANUP_HOOK(example_cleanup_hook,
                             "Example tests", "Example testcase",
                             cleanup_resources);

The registration names identify descriptors and must be unique within
the source file. Suite and testcase strings must match the XML manifest.
Cleanup should check which resources setup acquired and safely handle
early returns, skip results, and resources already freed by the test.
The testcase remains responsible for undoing its own configuration
changes and releasing resources outside the shared lifecycle.

The RME/RMM use of these hooks is described in:

.. toctree::
   :maxdepth: 1

   implementing-tests/realm-tests

Template Test Code
------------------

Some template test code is provided in ``tftf/tests/template_tests``. It can be
used as a starting point for developing new tests. Template code for both
single-core and multi-core tests is provided.

Build System Integration
------------------------

All test code is located under the ``tftf/tests`` directory. Tests are usually
divided into categories represented as sub-directories under ``tftf/tests/``.

The source file implementing the new test code should be added to the
appropriate tests makefile, see `.*mk` files under ``tftf/tests``.

The new test code should also appear in a tests manifest, see ``*.xml`` files
under ``tftf/tests``. A unique name and test function must be provided. An
optional description may be provided as well.

For example, to create a test case named "``Foo test case``", whose test
function is ``foo()``, add the following line in the tests manifest:

::

    <testcase name="Foo test case" function="foo" />

A testcase must be part of a testsuite. The ``testcase`` XML node above must be
inside a ``testsuite`` XML node. A unique name and a description must be
provided for the testsuite.

For example, to create a test suite named "``Bar test suite``", whose
description is: "``An example test suite``", add the following 2 lines:

::

    <testsuite name="Bar test suite" description="An example test suite">
    </testsuite>

See the template test manifest for reference: ``tftf/tests/tests-template.xml``.

--------------

*Copyright (c) 2018-2026, Arm Limited. All rights reserved.*
