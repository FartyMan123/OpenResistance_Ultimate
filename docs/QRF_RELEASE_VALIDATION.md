# Chicken QRF and attack release validation

**Decision: native release approval is blocked pending gameplay acceptance.** The user reports successful compilation on 28 October 2026. The source audit, mocked policy tests and installed-resource checks support the changes below. Driving, flight, actual landings and packaged dedicated-server replication remain unverified. No route is labelled as driven or flown.

Scope: Chicken only, both enemy roles, ordinary capture QRFs, trusted attacks, automatic Occupier attacks and MARS. Base revision: `6c03716ff3cdc573f56b8f428f436c249afc1d52`, branch `chicken-dev`, plus the accompanying working-tree changes. The requested work budget was two hours; simulated campaign ticks and assertion counts are not elapsed native gameplay time.

## Evidence and repeatable checks

- [Automated results and source hashes](validation/QRF_AUTOMATED_RESULTS.json).
- [All 350 configured response cells](validation/CHICKEN_RESPONSE_COVERAGE.csv): two armies × seven families × five Readiness levels × five Crackdown levels. 210 cells resolve an available response; the remaining 140 are intentionally locked. Availability does not establish affordability, current origin ownership, free live capacity or successful native deployment.
- [All 860 directed origin/target pairs](validation/CHICKEN_OPERATION_ROUTES.csv), [placed locations](validation/CHICKEN_OPERATION_LOCATIONS.csv) and [generation metadata](validation/CHICKEN_OPERATION_ROUTES.json). These are authored endpoints, not a reconstruction of Everon's complete road graph.

Node is absent on the audit machine. JavaScript suites were executed in the available V8 host with repository source snapshots, synchronous `fs`/`path` adapters and prototype restoration between suites. Those adapters do not provide Enfusion services. The Node runner below provides separate-process execution on a machine with Node installed; that runner itself was not executed through Node here.

```powershell
node tools/Validate-QRFRelease.js
node tools/Test-MarsAndAttacks.js --export-coverage
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Test-QRFSourceStructure.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Test-ForcePoolAssets.ps1 -ResourceDatabasePath '<base-game resourceDatabase.rdb>'
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Test-ReaperAssets.ps1 -ResourceDatabasePath '<REAPER resourceDatabase.rdb>'
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Export-ChickenOperationRoutes.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Test-QRFRouteImport.ps1
git diff --check
```

Installed-resource validation covers 27 base-game GUIDs, navigation bindings and faction profiles, all 42 recipes and the 50 campaign army/Readiness/Crackdown budget combinations. All five REAPER pilot/flight-waypoint references match the installed REAPER database. These checks do not validate native group materialization, weapon behavior or aircraft control. Source-structure checks are lexical checks, not compilation.

The three MARS, Black Market and setup layout GUIDs match their metadata, script/config references and the current local Workbench resource database. Their metadata remains hand-authored; actual widget loading and rendering remain unverified. Repeat that check after resource discovery with `tools/Test-OperationUIResources.ps1`.

## Confirmed defects fixed

| Area | Failure before the change | Correction and evidence |
|---|---|---|
| Airborne boarding | Accepted seat requests immediately marked the package mounted; partial boarding could also fail instead of waiting for asynchronous occupancy. | Both pilots must occupy their own pilot compartments and every passenger must occupy the transport. Initial teleports permit paused simulation. Repeated checks preserve players, occupants and other actors' reservations. The existing spawn deadline bounds waiting. Release lifecycle regressions exercise delayed and partial occupancy. |
| Aircraft readiness | A package could lose a pilot, passenger or aircraft while waiting for the other lead elements, leaving a stale ready flag. | Dispatch rechecks aircraft health, living personnel and verified boarding before releasing physics, issuing flight orders or repositioning either aircraft. Seven regressions cover loss and healthy dispatch. |
| Attack rally | `BeginStaging` issued rally orders, but no ground update marked arrivals or called the release path. Route recovery could bypass the intended rally. | Ground tactical updates require proximity and low speed, wait for all staged packages or the deadline, and release each package once. Withdrawal takes priority. |
| Defender intel | The unused assault helper calculated a centroid directly from hidden defender world positions. Activating the rally path would have exposed those positions. | Assault intel uses only the existing role-, vicinity- and age-filtered combat-report authority. No fresh report means unknown defenders. |
| Town pacification | The support policy existed without a call path and compared the army only against Resistance forces. | The shared five-second commander timer calls the settlement authority once per operation after physical occupation. All hostile military roles count against the occupying army; ties and withdrawal cannot advance support. |
| Attack eligibility events | A subscriber could synchronously resolve an ACTIVE capture QRF before the territory policy observed ACTIVE. | Record eligibility before invoking subscribers. Nested and duplicate terminal callbacks retain one completion timestamp. |
| Debug combat evidence | Damage to a MARS force used the normal strategic-evidence default, despite debug dispatch bypassing campaign scaling. | Debug damage retains a contact snapshot but explicitly suppresses Crackdown evidence. Ordinary campaign damage still escalates. Debug actors can still affect the physical world and canonical territory; MARS is not an isolated simulation. |
| Debug authority | A shipped MARS credential and an enabled admin menu were sufficient to request debug effects. | `AROR_AdminAccess` requires administrator/session-administrator roles in packaged sessions. MARS and all six admin server mutations enforce it. Menu actions and teleport selection recheck local access. Workbench retains testing access. Native permission replication still needs acceptance. |
| Scheduler input | An unsupported operation-type enum could create a record never processed by either scheduler pass. | Reject unsupported types before allocating an ID, cooldown or operation slot. |
| Withdrawal | A contested friendly installation could be selected as a safe replacement origin. | Contested sites are excluded; no eligible destination leaves safe recovery false. |
| Configuration | Newly active attack staging, town support and origin-cost parameters were missing from tactical validation. | Validate their documented ranges before tactical execution and native route probes. |
| Harnesses | Missing dependencies, obsolete UI bindings/wording, stale serializer expectations, incorrect reference initialization, an unhandled boolean parameter and strict prefab rounding assumptions obscured results. | Repair the harnesses while retaining behavioral checks. Layout checks now cover grouped Workbench entities and millimetre authoring precision. |

The scheduler soak uses 10,000 **simulated** ticks, 2,935 admitted/deduplicated requests, 27,065 rejected requests and 9,349 notifications. It checks global attack capacity, per-role QRF capacity, conflicting targets, bounded terminal history and nonrecursive notification delivery. It is not a real-time server performance or network soak. Many assertions in the lifecycle suite are repeated state invariants; the total is not a count of distinct gameplay scenarios.

Ten intentional source mutations were also detected: paused boarding, unready reserve dispatch, missing rally processing, disconnected support, late eligibility recording, debug escalation, missing admin authorization, duplicate collateral release, airborne group undercount and invalid operation admission. These run against isolated snapshots without editing game sources. The mutation pass exposed a cooldown masking the invalid-type test; that test now exercises the type guard directly. The pure mutation runner was executed in V8; its Node `vm` adapter remains to be exercised with the Node command above.

The [site-readiness audit](validation/CHICKEN_SITE_READINESS.csv) includes all 26 military locations in activation-overlap estimates and reports four legacy-catalogue transform differences: Pinewood Lake, Chotain Industrial, Old Wood and Andres Beacon. The world geometry was preserved. Resolve catalogue/generator drift before regenerating location layers. Several overlaps estimate 102 garrison personnel, so capacity pacing and populated-server performance need particular attention. The Invader policy model now reads actual placed military transforms and all three ports, rather than the older 23-site subset.

## Joint airborne and ground responses

`AIR_GROUND = 7` is appended to the response enum. Existing numeric values and recipe IDs are retained. Chicken enables `m_bAllowJointResponses`; its shared default is off. Airborne execution and tactical coordination must also be enabled.

Each joint recipe starts with exactly one complete transport/fireteam/gunship package, followed by its ground elements. Both roles receive tiers 1–5, unlocked at Readiness 5. Each list is deployed once; Crackdown does not multiply the list again.

| Tier | Ground elements accompanying the air pair | Lead / reserve packages | US / USSR funds required |
|---|---|---:|---:|
| 1 | Truck | 2 / 0 | 4,560 / 4,760 |
| 2 | Truck + APC/IFV | 3 / 0 | 5,620 / 5,720 |
| 3 | Truck + APC/IFV + armour | 2 / 2 | 6,530 / 6,280 |
| 4 | Truck + 2 APCs/IFVs + armour | 3 / 2 | 7,590 / 7,240 |
| 5 | 2 trucks + 2 APCs/IFVs + armour + light support | 4 / 3 | 8,400 / 7,950 |

One operation owns the complete funding account, virtual reservations, tactical roster and outcome. All required lead packages must be physically ready before dispatch. Ground and airborne reserves share construction pacing. Pending aircraft cannot be dispatched through the ground path. Joint attacks hold transport insertion at the air approach until the ground rally releases; ordinary joint QRFs retain immediate insertion. Native gunship perception and firing policy remain responsible for engagement.

At equal authored tier the campaign selector prefers an eligible, affordable joint recipe when enabled; it can retain a higher-tier ground recipe or use an affordable ground alternative. The emergency unsecured capture response remains ground-only. Ground stock-provider previews exclude joint recipes because their provider does not assert aircraft availability. Full-force funding is retained even while later waves wait.

The path deliberately supports one air pair per joint recipe. Independent transport/gunship counts, shared gunships, unescorted transports, specialist anti-armour loadouts and the complete proposed [40-entry design roster](QRF_RESPONSE_ROSTER.md) remain outside implemented coverage. The design CSV is not an executable catalogue.

**Extender impact:** `AROR_ForceProfile.GetGroupBudget()` now correctly reports three groups for an airborne package; `GetVehicleBudget()` reports two aircraft. `DispatchReadyPackage` and `StartJointResponse` are executor hooks. Ordinary recipes remain ground-only; joint recipes use the explicit family and air-first ordering. The recipe limit accommodates the expanded catalogue. Admin debug access in packaged sessions now requires native administrator roles. No save-format change is introduced; live joint operations remain session-only.

Town security now applies the existing configured support policy, whose default is six percentage points per minute while the conditions hold. This changes campaign behavior compared with the previously disconnected helper and requires balance acceptance.

## Native route sweep

Chicken has 44 placed locations and 20 possible military launch sites: **860 directed non-self pairs**, including **500 military attack pairs** and **60 port-target pairs**. Both responding armies can acquire different origins during a campaign, so the matrix includes prospective ownership states. Its preferred-origin-distance flags derive from the actual source/layer configuration, currently 600 m. Endpoint height differences cannot reveal intermediate slopes, bridges, water crossings or obstacles.

After Script Editor compilation, run an idle disposable Chicken campaign and invoke:

```c
AROR_QRFRouteAudit.Start_S();
// To cancel:
AROR_QRFRouteAudit.Stop_S();
```

Use the existing Script Editor/debug invocation mechanism for static smoke tests. The audit is Workbench-only and server-only. Do not use it as a background task during combat. It stops when a gameplay operation becomes outstanding and clears its timer/references on executor deletion.

The sweep first advances the production five-pad LZ search for each target at no more than two radial seeds per tick. It then performs one directed pair per second using the production bounded ground launch/approach planners and all airborne launch formation sizes 1–5. Escort headings follow the selected landing pads; cached pads are rechecked before launch probes. At the current 121-candidate limit the worst case is roughly 45 minutes for LZ searches plus 15 minutes for directed pairs, plus scheduler delays. It does not spawn actors, spend money, change territory or give AI orders; navmesh tiles may be loaded by the existing foot-dismount checks.

Keep the log and import it separately:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Import-QRFRouteAudit.ps1 -LogPath '<Workbench log>'
```

The importer uses the latest audit start, checks topology counts, rejects unknown origin/target IDs and preserves missing results as `NOT_RUN`. Ground planning, air launch planning and pad counts are separate columns; unavailable landing sites produce `NO_SAFE_LZ` for air launch planning. It never marks driving, actual landing or dedicated-server checks passed. Synthetic importer fixtures were written only to temporary files; no synthetic planning result is included as native evidence.

A planning PASS is a snapshot for one ground package and the configured air launch formations. It does not establish space for a complete ground convoy, a navigable infantry route from every outer LZ, vehicle turning clearance, bridge suitability, gunship engagement or successful travel. Player clearance, loaded tiles, garrisons and dynamic obstructions can change results. Repeat failures after tiles load and with player bodies away from the origin; moving the Game Master camera alone does not change player clearance. Archive the current source hashes with the native log.

## Release acceptance still required

| Gate | Required evidence | Current result |
|---|---|---|
| Compilation | Compile current scripts; verify packaged and Workbench variants. | PASS, user-reported; build variants not separately specified |
| Resource loading | Discover layouts and metadata; reload Chicken and check widget loading. | NOT_RUN |
| Memory-only native smoke tests | Existing operation/pool/strategy/tactical helpers plus `AROR_JointResponseSmokeTests.Run_S()`. | NOT_RUN |
| Route planning | Sweep all 860 pairs and 44 landing targets; triage failures, especially ports. | NOT_RUN |
| Ground travel | Both armies, every authored recipe tier, driver/passenger seating, full lead wave, convoy spacing, dismount, infantry arrival and return. | NOT_RUN |
| Air travel | Both armies, package counts 1–5, touchdown/normal exits, gunship support, alternate LZs, egress, retention and live-aircraft cap. | NOT_RUN |
| Joint responses | Both armies and tiers 1–5 as capture QRFs and attacks; shared lead barrier, ground rally/air hold, reserve loss, and canonical capture. | NOT_RUN |
| Military attack lifecycle | Grace/recovery/rest boundaries, failed-spawn exclusion, one global attack, target deletion/change, loss of origin and insufficient funds. | NOT_RUN |
| Settlement security | Support-only control, physical presence, both hostile armies, no hidden-position targeting and no per-package support multiplication. | NOT_RUN |
| Multiplayer authority | Dedicated server, admin and ordinary client, rejected raw debug requests, role revocation, reconnect/JIP and matching script/layout versions. | NOT_RUN |
| Save compatibility | Versions 1/2/3, funds/debt/Readiness/Crackdown restoration; documented session reset of operations and attack eligibility. | NOT_RUN |
| Runtime cleanup/performance | Cancellation during materialization, dormant groups, late agents, player takeover, aircraft loss, nearby bodies/loot and a populated multi-hour session. | NOT_RUN |

Prioritize Lamentin Port from Sawmill and Old Wood, North Ridge, all three ports, the longest directed pairs, low-elevation endpoints, nearby origins and routes with large endpoint height differences. Include road barriers/bridges, steep approaches, narrow entrances, moving obstructions and outer-pad infantry approaches. Those terrain features need native observation; this audit does not infer them from straight-line geometry.

Use a disposable fixture with shortened attack timers for boundary tests, then restore 900-second capture grace, 300-second post-QRF recovery and 900-second rest and observe the real cadence. An arrival without ownership change must not succeed. A spawn failure must not unlock attacks. Save/reload must not fabricate a historical capture QRF. No safe origin, insufficient capacity or an unsafe pad must produce a finite refusal/failure with reconciled reservations.

A two-hour session can cover the planning sweep and selected physical regressions. It cannot honestly certify every dynamically obstructed route and every response in multiplayer. Preserve the untested cells and continue native acceptance before labelling Chicken release-ready. The experimental REAPER dependency remains required; battlefield loot, live-operation persistence and physical military stock retain their documented limitations.
