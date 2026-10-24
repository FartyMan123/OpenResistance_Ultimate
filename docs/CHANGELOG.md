# Changelog

## Unreleased - QRF tactical command and finite waves (2026-10-24)

- Add a shared server tactical commander for US-backed Occupiers and USSR-backed Invaders, using existing Operations, factions, location ownership, native group AI and REAPER. Coordinate report-driven investigation, bounded infantry sectors, survivor/new-wave redistribution, ground delivery, separate IFV/armour road support and landed air infantry. Retask from time-limited position snapshots; no live player tracking, building-clearance controller or new territory authority is introduced.
- Add optional type-balanced lead waves and finite actor-free reserves for larger responses. Reserve the complete original allocation, construct at most one ground element or air pair per executor tick, admit later elements under delay/live caps and expire unused reserves. Preserve healthy active elements after reinforcement failure and prevent repeat construction/duplicate activation. Air pairs require two free aircraft slots; undersized lead-wave limits reject configuration before spawning.
- Add ownership/task-duration/casualty withdrawal and ground retention requiring minimum age AND distance from every player. Retained surviving troops still occupy the live ground ceiling and all retained forces retain aggregate capacity. Keep air boarding holds until flight dispatch, share infantry search orders, and exclude successful aircraft departures from troop-casualty accounting. Player-control checks remain in force.
- Enable tactical coordination on Chicken's existing tier-1 truck/LAV fixture, retaining one wave for its two elements. Leave Invader admission disabled, Invader targets empty and the REAPER override off. No economy provider, stock spending, production, territory/factory setup or automatic USSR caller is added.
- Add Workbench tactical smoke tests, offline source-adapted policy/lifecycle checks and tactical fixture validation; document behavior, wave tables, settings, remaining mixed-domain/persistence limitations and Play acceptance in `QRF_TACTICAL_COMMANDER.md`. Native compilation, movement/flight, FPS and dedicated/multiclient acceptance remain outstanding.
- Extender impact: tactical execution defaults OFF outside the explicit Chicken override. Opt-in scenarios gain deferred reservations, bounded live deployment and terminal retention; aggregate budget may stay occupied near players. Recipe tier and element count remain separate, and runtime/session state is not saved. Existing non-tactical paths remain available.

## Unreleased — Commanders and QRF foundation (2026-10-23)

### Integration with master-dev

- Preserve Rowan's core factions, forced Resistance spawning and setup-flow updates while retaining the commander/scheduler components.
- Use the shared `AROR_FactionRolesComponent` as the authority for commander faction bindings. Remove the duplicate commander-folder role enum; the canonical definition lives under `Scripts/Game/Factions/`.
- Retrofit Chicken onto the same core AROR faction keys and Resistance loadout as original Everon. Its defenders resolve the shared defending role; profiles declare campaign faction separately from native US/USSR assets.
- Initialize operation-owned vehicle/member affiliations once from an empty, matching, or explicitly declared native asset faction. Initialize empty groups or use the native server `SetFaction` API for assigned groups before requesting members. Unexpected affiliations and later faction changes still fail validation.
- Extenders must move commander-manager faction attributes to `AROR_FactionRolesComponent`; evaluation interval remains on the commander manager. Recompile and reload prefabs/worlds before testing this integration.

### Added

- Add a non-dispatching Invader ground QRF planner with authored tier limits/downgrade policy, expiring faction/role readiness snapshots, repeated-profile stock accounting and aggregate runtime capacity checks. Expose a server readiness-provider registration/preview seam that validates target ownership, friendly outpost origin and route/arrival constraints without mutating provider data. No default resource provider, economy spending, automatic caller or Invader trigger is added. Fifteen source-adapted offline scenarios passed with mocked dependencies; Workbench smoke tests are added but unrun, and native compilation/provider integration remain outstanding.

- Stage the USSR-backed Invader QRF framework with five authored Ural/BTR/BRDM ground tiers, role-specific ground recipe selection and the existing shared Mi-8/REAPER support paths. Default Invader operation admission OFF and explicitly disable it in Chicken, rejecting Invader attack/QRF requests before operation creation; leave automatic/test targets empty. Preserve the Occupier truck/LAV fixture. Extend capacity/configuration and disabled-gate checks. No territories, factories, Invader defenders, production or triggers are added. Extender impact: Invader operations now require explicit opt-in; existing role mappings and profile IDs are unchanged. Compilation and Invader runtime acceptance remain outstanding.

- Add authored ground QRF recipes resolving ordered existing profiles under one operation, with atomic aggregate runtime reservations, separate tier/element count, complete pre-dispatch readiness and survivor behavior after active losses. Configure Chicken for combined_arms_l1: US M923 truck plus LAV-25 with eight infantry/four crew, four groups/eighteen reserved slots/two vehicles; provide the Ural/BTR role equivalent. Disable the air override and clear the old forced US profile. Add recipe validation/regressions and extend installed-asset fixture checks. Extenders can opt into recipe configuration; homogeneous behavior is retained. Workbench compilation and mixed-force Play acceptance remain outstanding.

- Design a 40-entry QRF roster: motorized, mechanized, armour counterattack, air assault, combined arms, anti-armour, reinforce-and-hold, and search/containment, each with authored levels 1-5. Include a Markdown table and CSV with stable IDs/nominal accounting, location/threat suitability, required asset/crew/supply/ETA gates, and a future resource-reservation/selection contract. Mixed execution, stock-aware selection and composite tier semantics remain planned; no runtime scripts, spawn counts or economy balances change.

- Add role-scoped, expiring allied combat reports for gunship support, fed by validated assigned-defender damage and operation-owned units even when QRF admission is on cooldown. Gunships investigate recent nearby snapshots and vary search passes without live player tracking; native hostile-target and friendly-clearance filters remain. Separate gunship altitude bands from transports and add predictive proximity yielding with stable gunship priority. Ground and airborne QRFs select the nearest eligible friendly OUTPOST, falling back to configured role origins. Append OUTPOST to the enum and existing military capture/recapture classification; existing enum values stay stable. Offline source-adapted checks passed for five traffic predictions, eight contact-policy cases and four origin-policy cases with engine dependencies mocked. New integration still requires Workbench compile/Play and dedicated/multiclient verification.

- Set Chicken's Workbench response level to 5 and make REAPER dispatch honor levels 1–5: five transports/fireteams plus five gunships for this test. Reserve the complete fleet and select a shared LZ, gate activation on every package being boarded, and retain healthy packages after individual active losses. Add package-specific launch lanes, altitude bands, gunship pass phases and infantry sectors; extend configuration validation to cover airborne fleet budgets. Requires fresh Workbench compilation and testing.

- Extend REAPER QRFs with closest sampled clear-town LZ selection, offset ingress guidance, last-contact infantry investigation/patrol, transport takeoff/egress with all-player distance checks, and moving gunship passes. Use documented REAPER controller/waypoint interfaces and a perceived-target filter for campaign hostility and nearby non-hostiles. New backend signatures and behavior require fresh Workbench compilation/testing.
- Retain active air-package casualties after pilot/aircraft loss; allow surviving grounded passengers and delivered infantry to continue. Terminal air forces keep ownership/capacity until a minimum retention period and all-player distance checks permit removal; world unload flushes retained records. This is session retention, not saved persistence.

- Add a direct REAPER QRF experiment on the existing Chicken defender/scheduler path: one transport/fireteam and one gunship, faction checks, three-group/two-aircraft reservations, occupied midair insertion with native airborne engine startup, Move/Land orders and landing/exit-gated infantry orders. An off switch restores native ground dispatch. The separate disabled flight probe remains diagnostic; the first run established basic flight/landing/unloading, while the revised engagement/formation/egress/retention behavior and weapon filtering remain unverified.

- Limit airborne concurrency to one operation with one to five complete packages and a shared reserved LZ; transport now departs after unloading and gunship flies moving passes before egress. Bounded candidate LZ checks are included; full route certification and persistence are deferred. Normal airborne selection remains unchanged. At Jasper's request this experimental branch now adds REAPER to the normal project dependencies and moves the adapter into core AROR scripts; Chicken enables the switch explicitly, while other executors default off. Ground dispatch can be restored without removing the dependency.

- Shared QRF tactical search plans: snapshot validated attacker reports, investigate recent contact, then assign separate marker patrol sectors and spaced road stops to each vehicle package. No live attacker tracking; stale/missing reports fall back to town patrol. These latest changes require Workbench compilation and testing.

- Three configurable role-based commander contexts: player-directed Resistance and AI-directed Occupiers and Invaders. Bindings come from Rowan's shared role component, defaulting to `AROR_RESISTANCE`, `AROR_OCCUPIERS` and `AROR_INVADERS`, with FIA/US/USSR assets respectively.
- Server-owned attack and QRF scheduling, independent role cooldowns/capacity, target deduplication, bounded operation history and terminal cleanup.
- Weighted force pools and response levels 1–5. Each level deploys that many complete packages, with the whole response reserved before spawning.
- Ground execution for motorized, mechanized and wheeled-armour packages, including queued crew/infantry spawning, boarding, deployment waypoints and individual package-loss handling.
- Hostile-damage reporting from explicitly assigned defenders, with faction, instigator and location validation.
- Optional bounded mounted QRF town searches using reachable road points and native AI perception.
- Deployment faction audits, continuing vehicle-faction validation and optional same-faction damage diagnostics.
- Separate Everon_Chicken test world with Provins defenders, navigation bindings, perception manager and configurable US/USSR force pools. The current fixture enables the REAPER test at level 5 (five transport/fireteam/gunship packages); disabling it falls back to the forced US mechanized profile at level 5.
- Workbench smoke-test helpers, installed-resource/configuration validation and setup/testing guides.

### Fixed

- Plan individual REAPER transport landing sites across the town vicinity (400 m default search, expanding to town radius plus 150 m, capped at 800 m), with 90 m configurable pad/corridor separation. Distinct pads can land concurrently; if fewer clear pads exist, only transports sharing a pad queue, preserving the full response. Search remains cached at two candidates per tick and 73 total. Blocked pads receive package-local alternate searches with a shared fleet-wide budget and stage timeout. Eleven source-adapted offline planner scenarios passed with mocked geometry/native acceptance; extended Workbench smoke tests, actual flight and FPS remain unverified.

- Reduce transport exposure in the REAPER QRF: grant direct landing clearance before the approach/hover gate; move queued holding points outside town (minimum 700 m behind the LZ); expose 160 km/h ingress, 60 m AGL, two stable touchdown ticks and 180 km/h departure as designer attributes, explicitly set in Chicken. Request animated passenger exits on the stable-touchdown tick and retain the actual-exit gate before takeoff. Match escort ingress commands to transport speed/height. The previous run logged about 68 seconds from Land order to stable touchdown; actual speed/descent remain REAPER-controlled and the new tuning needs Workbench compilation and timing verification.

- Replace repeated level-5 full landing/assembly scans with a cached shared-LZ search: at most two candidates per executor tick, 73 total, then stop on success or exhaustion. Remove ground assembly-pad searches; validate air insertion volumes, hold aircraft while boarding and construct at most one transport/gunship pair per tick. Transports hold separately and take turns through the landing/departure corridor, with incremental alternate-LZ searches if blocked. Level 5 remains five complete packages. Seven source-adapted offline planner scenarios passed with native terrain/trace acceptance mocked; Workbench regression helpers are added but unrun. Compilation, actual flight and FPS remain unverified.

- Fix REAPER executor compilation: split oversized validation/cleanup/faction formulas into independent checks and replace the reserved `owned` loop identifier. Fresh Workbench compilation is still required.

- Make REAPER discoverable through Workbench's standard addon folder using a local junction to its installed files. The normal project can now open without the CMD launcher or separate project. The old launcher remains available only for standalone diagnostics.

- Replace an unsupported remainder operator in the coordinated search plan that blocked Game-script compilation and left Play running the older patrol implementation. A fresh successful compile is still required.

- Designer force-profile/response-level arrays being overwritten during initialization.
- Duplicate inherited commander/scheduler components caused by inconsistent component IDs.
- Recursive operation-event invocation, empty standalone defender affiliations, and the agent-added callback signature in source.
- Premature loss detection for groups still materializing through the engine queue.

### Validation and remaining work

- Configuration validation passes for 25 installed resource GUIDs, three navigation bindings, eight profiles and five response levels; staged whitespace checks pass.
- Observed Play tests confirm hostile-hit QRF dispatch, motorized spawning/boarding/activation, and all three level-3 LAV packages boarding and becoming ACTIVE. Jasper observed the infantry truck arrive; the latest logs also show vehicle movement.
- The latest recorded run still used older compiled scripts and reported the previous agent-added callback error. Current callback, town-search and faction-diagnostic changes require a fresh Workbench compile and Play test. Apparent LAV friendly fire is not yet confirmed or excluded.
- Airborne transport/gunship profiles remain disabled. The first direct-dispatch run confirmed boarding, flight, landing and unloading, but showed friendly explosive damage and immediate terminal cleanup. New engagement, LZ, formation, egress and retention changes require fresh compilation and runtime verification.
- Dedicated-server/multiclient acceptance, full deployment/dismount/search behavior, durable persistence, territory outcomes and economy integration remain outstanding.

### Extender impact

- This development branch now requires REAPER (`69A4D664A6284E06`) at project load. This is an explicit experimental exception to the base-game-only architecture; turning off Reaper QRF Test restores ground behavior but does not unload the dependency.

- The shared `AROR_GameMode_Base.et` now inherits commander and operation-manager components. Extenders should override these inherited components rather than add duplicates, and keep role bindings consistent with their scenario factions and loadouts.
- Force profiles must match configured commander factions. Ground profiles require matching crew, infantry and vehicle resources; unsupported airborne profiles are excluded.
- Executor pools and test fixtures are configured in Everon_Chicken; inheriting the shared game-mode template alone does not enable this test executor.
- Operation state is server-owned and session-local. No persistent save schema or client operation UI is introduced.

Architecture and project guidance are recorded in `ARCHITECTURE.md` and `CLAUDE.md`, with synchronized copies in `AGENTS.md`.
