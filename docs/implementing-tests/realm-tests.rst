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
2. Initialise the host Realm heap used for test allocations and memory
   donated to the RMM.
3. Select the tracking size, configure the RMM with 4 KB granules,
   and activate it if it is inactive. The common setup selects either
   1 GB or 2 MB tracking regions for each testcase.
4. Begin the testcase pool lifecycle so tracking metadata can be
   reclaimed at cleanup.

If RME or the RMI interface is unavailable, setup leaves the testcase to
perform its usual skip checks. The Test Realm Payload (TRP), which
reports RMI version zero, does not use the RMM activation path.

The setup hook prepares the host environment; the test still creates
and destroys its own Realms, RECs, mappings, and device objects.
``host_rmm_activate()`` checks the RMM state and returns successfully if
it is already active.

Cleanup and a test-controlled restart
-------------------------------------

``host_realm_testcase_cleanup()`` reclaims testcase tracking metadata
and then deactivates the RMM. The known Realm heap range is scanned
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
3. Run the test's inactive-state checks or configure and activate the
   RMM through its own RMI sequence. Before an activation that can
   donate memory, initialise the heap with ``host_realm_pool_init()``.
   To reuse the default configuration, call ``host_rmm_activate()``.
4. After activation succeeds, call ``host_realm_pool_testcase_begin()``
   before creating objects or fine tracking regions for the new
   lifecycle. Check every return value and release the test's objects
   before leaving its body.

Any test-selected tracking layout must agree with the region size used
by the host tracking helpers.

Framework cleanup still runs when the test returns. Releasing an
inactive testcase pool or deactivating an already inactive RMM succeeds,
so an early cleanup does not suppress the final cleanup. A test that
remains inactive need not restart the pool lifecycle.

--------------

*Copyright (c) 2026, Arm Limited. All rights reserved.*
