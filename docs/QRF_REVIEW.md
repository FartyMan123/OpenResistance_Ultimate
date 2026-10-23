# QRF review — October 23, 2026

This records a source/configuration review, not a successful gameplay test. The last inspected live run triggered a QRF and selected `us_motorized`, then failed vehicle faction validation and raised a recursive ScriptInvoker exception. Its stack trace points to the older direct invocation; it does not validate the corrected notification queue.

## Pass 1: notifications and cleanup

- Operation changes made inside a notification update state immediately and enqueue their event until the current invocation returns. The invoker is not called recursively.
- Traced a spawn failure through its terminal event, package/reservation removal and cleanup. Terminal callbacks remain idempotent when no package remains.
- Added a Workbench regression probe that resolves a second operation from the first one's cancellation notification and checks exactly one follow-up notification with no recursive callback. This test is written but has not run.

## Pass 2: factions and spawning

- Both placed defenders explicitly initialize `US` before attaching damage monitoring. Live logs confirmed that assignment and a hostile hit reaching QRF dispatch.
- Newly created operation vehicles initialize empty affiliation to the owning commander's faction. Conflicting affiliations still fail validation.
- Crew and infantry groups initialize their faction before requesting members. Newly registered members initialize an empty affiliation once; periodic counting does not continuously reset later faction changes.
- Ground groups do not count as eliminated while awaiting their first members. Crew and infantry track materialization separately, so one element appearing does not make another queued element count as defeated.
- Reviewed level-one reservation, member requests, activation barrier, seating, separate crew/passenger orders, dismount and terminal cleanup paths. Actual engine queueing, seats and driving remain unverified.

## Pass 3: Chicken configuration

- Verified 21 resource GUIDs against the installed read-only index, including the Everon AIWorld prefab.
- Verified eight unique profiles, five configured levels, two disabled airborne profiles, and enough configured capacity for supported packages at every level.
- Confirmed matching inherited commander/scheduler component IDs and one explicit Chicken AIWorld placement.
- Confirmed a level-one `us_motorized` test override and blank simulated-report targets.
- Checked Operations source delimiters, previously rejected syntax, whitespace, and synchronized project guidance. These checks do not compile Enforce Script.

## Remaining runtime acceptance

Recompile and reload Chicken, preserving unsaved edits. Verify no missing-AIWorld error, both defenders assigned US, one real hit creates one QRF, then profile selection, `Ground package queued`, ACTIVE, driving and dismount. Also run the scheduler smoke tests with the executor disabled in a separate fresh disposable session to exercise the nested-notification regression. A failure/cancellation test must clean up the whole response without a VM exception. Launch terrain, route accessibility and dedicated-server behavior still require engine testing. The existing initial-save failure is separate and unresolved.
