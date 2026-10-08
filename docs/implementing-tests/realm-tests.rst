Realm Test Lifecycle
====================

RME-enabled tests share basic host setup before exercising the Realm
Management Monitor (RMM). Testcase setup and cleanup hooks put this
common lifecycle around the test body, so each test can concentrate on
its resources and assertions. Build and packaging instructions are in
:doc:`../getting_started/build`, which also links to the TF-A RME guide.

Framework hooks
---------------

The RME lifecycle uses the generic :ref:`design_testcase_hooks`
capability. See :ref:`implementing_testcase_hooks` for callback
registration, suite and testcase filters, and source build integration.
The framework runs these callbacks on the lead CPU; tests synchronise
secondary CPUs and release their RMM objects before returning.

RME hook selection
------------------

The shared RME hooks are registered by ``host_realm_helper.c`` for the
Realm/RMM-dependent suites: ``Realm payload at EL1``, ``RMI and SPM
tests``, ``SP exceptions``, and ``Invalid memory access with RME
extension``. Within ``SMMUv3 tests``, they are registered only for
``Check secure peripheral access to a realm region is aborted``.

Thus, RME-enabled tests selected by these registrations receive the
common lifecycle. Enabling Realm payload support does not apply RMM
setup to every test in a broad build such as ``TESTS=standard`` or
``TESTS=spm``. RMM-dependent suites or testcases should register both
hooks with the appropriate scope.

Basic setup and RMM activation
------------------------------

``host_realm_testcase_setup()`` establishes the default environment:

1. Check RME support, reset the RMI register-comparison result, and
   probe RMI version. Tests retain their own feature and skip checks.
2. Retry any previous pool cleanup and deactivate the previous RMM
   lifecycle. Stop if it still owns resources; do not change tracking
   size or reset its allocation cursor.
3. Initialise the host Realm heap used for test allocations and memory
   donated to the RMM.
4. Select the tracking size, configure the RMM with 4 KB granules,
   and activate it. The common setup selects either 1 GB or 2 MB
   tracking regions for each testcase.
5. Begin the testcase pool lifecycle so tracking metadata can be
   reclaimed at cleanup.

If RME or the RMI interface is unavailable, setup leaves the testcase to
perform its usual skip checks. The Test Realm Payload (TRP), which
reports RMI version zero, does not use the RMM activation path.
Both hooks reuse ``SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP()`` through
an availability check. Its skip result makes the hooks return success
without RMM lifecycle operations, leaving each test's own checks to
determine its skip or TRP result.

The RMM used with this lifecycle must implement ``RMI_RMM_STATE_GET``,
``RMI_RMM_CONFIG_GET``, ``RMI_RMM_CONFIG_SET``, ``RMI_RMM_ACTIVATE``, and
``RMI_RMM_DEACTIVATE``, and support both tracking sizes with 4 KB granules.
Setup fails if the required lifecycle commands are unavailable.

The setup hook prepares the host environment; the test still creates
and destroys its own Realms, RECs, mappings, and device objects.
``host_rmm_activate()`` checks the RMM state and returns successfully if
it is already active with the selected configuration. An active RMM with
a different configuration is rejected without changing the host helpers.

The common setup uses ``rand()`` to choose the layout before each testcase.
At ``LOG_LEVEL=40`` or higher, it logs the testcase name, selected size,
and configuration request/result.

Cleanup and a test-controlled restart
-------------------------------------

``host_realm_testcase_cleanup()`` reclaims testcase tracking metadata
and then deactivates the RMM only after reclamation succeeds. The known
Realm heap range is scanned
automatically. Fine tracking regions outside the heap, including regions
crossing its boundaries, are recorded explicitly and restored first.
The heap is reset only after its tracking metadata has been reclaimed.
Reinitialising the pool during an active testcase preserves its allocation
cursor, including when another RMM object prevents cleanup after the last
Realm is destroyed. Failed testcase cleanup retains the registry and cursor
for a retry and blocks pool reinitialisation and a new pool lifecycle until
reclamation succeeds.

A test that does not need the default active RMM, or that needs a
different configuration, can perform the same cleanup sequence at the
start of its body and restart the lifecycle itself. The registered
callbacks are private to ``host_realm_helper.c``; tests use their public
lifecycle helpers rather than calling the callbacks directly:

1. Synchronise participating CPUs and destroy any Realms or other RMM
   objects already created by the test.
2. Call ``host_realm_pool_testcase_release()`` to reclaim tracking
   metadata, then ``host_rmm_deactivate()`` to return the RMM to its
   inactive state. Stop and report failure if either operation fails.
3. Run the test's inactive-state checks. To restart, initialise the heap
   with ``host_realm_pool_init()`` and call
   ``host_rmm_activate_with_tracking_size()`` with either
   ``RMI_GRAN_4KB_TRACKING_REGION_SIZE_1GB`` or
   ``RMI_GRAN_4KB_TRACKING_REGION_SIZE_2MB``. This helper configures and
   activates the RMM and updates the host tracking helpers only on
   success. To reuse the last selected size, call ``host_rmm_activate()``.
4. After activation succeeds, call ``host_realm_pool_testcase_begin()``
   before creating objects or fine tracking regions for the new
   lifecycle. Check every return value and release the test's objects
   before leaving its body.

For example, a test can restart with 2 MB tracking before creating its
objects:

.. code:: c

   #include <host_realm_helper.h>
   #include <host_realm_rmi.h>
   #include <tftf.h>

   test_result_t test_realm_with_2mb_tracking(void)
   {
       if (!host_realm_pool_testcase_release() || !host_rmm_deactivate()) {
           return TEST_RESULT_FAIL;
       }
       if (!host_realm_pool_init() ||
           !host_rmm_activate_with_tracking_size(
               RMI_GRAN_4KB_TRACKING_REGION_SIZE_2MB) ||
           !host_realm_pool_testcase_begin()) {
           return TEST_RESULT_FAIL;
       }

       /* Create, exercise, and destroy the test's RMM objects here. */
       return TEST_RESULT_SUCCESS;
   }

Framework cleanup still runs when the test returns. Releasing an
inactive testcase pool or deactivating an already inactive RMM succeeds,
so an early cleanup does not suppress the final cleanup. A test that
remains inactive need not restart the pool lifecycle.

--------------

*Copyright (c) 2026, Arm Limited. All rights reserved.*
