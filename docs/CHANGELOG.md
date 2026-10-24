# Changelog

## Unreleased - Rowan integration and combined development build (2026-10-24)

### Branch reconciliation

- Commit the accumulated QRF, REAPER and tactical work on `chicken-dev` as `7a3b340` (`Add QRF tactical command and staged waves`).
- Merge Rowan's `master-dev` work through `99ad920`, including PRs #8, #9 and #10. Preserve both development histories without rebasing or force-pushing the shared branch.
- Rebuild the conflicted `AGENTS.md` architecture copy from current authoritative `CLAUDE.md` and merged `ARCHITECTURE.md`. Keep Economy, Phone and Admin Tools architecture alongside the QRF design. No project-guidance policy is rewritten.
- Keep the shared role authority: Resistance uses FIA assets, Occupiers/BLUFOR use US assets and Invaders/REDFOR use USSR assets. QRF code does not introduce another faction or location ownership registry.

### Rowan: campaign economy and persistence

- Bring in role-based faction funds, Resistance manpower and personal player money through `AROR_EconomyComponent` and its serializer. Faction balances are replicated; personal balances are sent through the player's controller and indexed by persistent identity.
- Preserve server-owned add/spend APIs and nonnegative balance checks. Save economy state through the existing game-mode persistence path and retain setup/new-campaign initialization.
- Bring in `AROR_IncomeComponent` and Rowan's latest 15-minute default payment interval. Held resource locations produce $500 each, multiplied by factory/port bonuses; held town/city population adds $5 per inhabitant outside that multiplier.
- Include the newer fixed external-support payment for Occupiers and Invaders ($500 per role per payment), independent of their held locations.
- Include the newer personal share for every online Resistance player: 10% of Resistance income, paid in addition to faction funds. Keep Resistance manpower income, setup-phase gating and income notifications.
- Rates remain designer attributes. These are campaign money/manpower systems; they do not yet supply military vehicle stock, crew availability, ammunition, route readiness or QRF purchase reservations.

### Rowan: phone, bank and administration

- Bring in the client phone menu, frame/app/status assets, menu registration and rebindable input context. Default opening action is Ctrl+Y for a living player when no other menu is open.
- Include the Bank app displaying personal money and Resistance funds, portrait/landscape transitions, home navigation, status bar and Escape handling. Money transfers and general app registration remain planned.
- Preserve the expanded Ctrl+T admin sub-menus, economy balance display, money-grant buttons and immediate-income test action alongside teleport, Game Master, support and notification tools.
- Keep the upstream setup-dialog and player-controller changes needed by these systems. Admin access restrictions remain future work; this merge does not claim to add them.

### QRFs: shared tactical command

- Add one server tactical commander per QRF operation for either opposing role. Existing faction strategic commanders still own scheduling; the tactical commander coordinates the already selected immutable response.
- Use recent validated allied combat reports to investigate last-known enemy positions, then search bounded town sectors. No live enemy entity tracking or omniscient player-position feed is added.
- Give dismounted infantry separate sectors, redistribute after losses/new waves and rate-limit retasking. Retain native AI perception, navigation and weapon use.
- Coordinate truck delivery and waiting, separate IFV/armour road cover, reported-threat dismount standoff and bounded ingress attempts. Air-inserted infantry uses the same tactical search controller.
- Add local/whole-force casualty withdrawal, target/origin ownership checks, safe return/hold behavior and bounded task duration. Building-by-building clearance, guaranteed convoy movement and coordinated suppression remain outside this implementation.

### QRFs: reinforcement waves and performance limits

- Split eligible tier-3+ responses with at least four elements into a type-balanced lead and finite reserve. Keep later elements as actor-free reservations rather than spawning them into a holding area.
- Preserve the full original group/personnel/vehicle allocation; later waves do not reroll the recipe, create extra replacements or consume untracked capacity.
- Limit construction to one ground element or transport/gunship pair per executor tick. Default live ceilings are six ground packages and six healthy aircraft, including retained survivors/healthy airframes.
- Normally release reserves after 60 seconds from lead dispatch, shorten eligibility to 15 seconds following package loss, and expire unspawned reserves after 300 seconds. Later elements join the existing ACTIVE operation.
- Prepared Invader ground tiers 3/4/5 divide into 3+2, 4+3 and 4+4 elements. Air level 5 divides into three pairs plus two reserved pairs; a pair requires two free aircraft slots.
- Hold capacity for retained forces near players. Waves bound deployment/construction work; they do not certify FPS or guarantee every reserve deploys before its deadline.

### QRFs: air operations, origins and retention

- Bring the direct REAPER experiment into the normal project, retaining separate diagnostic files and launcher for optional troubleshooting. The current normal fixture leaves flight dispatch disabled, but REAPER remains a load-time dependency on this experimental branch.
- Preserve incremental separated-pad landing searches, direct transport landing, stable-touchdown and actual passenger-exit gates, departure, report-driven gunship passes and predictive traffic yielding.
- Keep boarding holds for later-wave helicopters until flight dispatch. Successful aircraft departures do not count as inserted-infantry casualties, and disabled/retained healthy airframes do not silently free live slots.
- Select eligible nearest friendly outposts for ground/air QRF origins, with configured role coordinates as fallback. Append OUTPOST without changing existing location enum values and retain military capture classification.
- Keep nearby ground survivors/wrecks at least 120 seconds and until every player is 1,200 m away. Preserve the separate air retention policy and aggregate reservations. These are session policies, not durable saved operations.

### Worlds, fixtures and resource planning boundary

- Bring in Rowan's separate Everon test world, income-test location data and original Everon updates. Preserve the separate `AROR_Everon_Chicken` QRF fixture and its inherited shared game-mode systems.
- Keep Chicken on the authored US truck/LAV `combined_arms_l1` response: two vehicles, four groups, twelve actual personnel and eighteen reserved slots, with tactical coordination enabled. This small tier-1 response stays in one wave.
- Keep the REAPER override off, Invader operation admission off and Invader automatic/simulated targets empty. Stage USSR Ural/BTR/BRDM tiers and Mi-8 profiles without adding an Invader gameplay trigger.
- Keep the resource-aware QRF readiness-provider preview read-only and disconnected from dispatch/spending. Rowan's balances and income are preserved as the economy authority; no parallel economy, stock generator or automatic QRF funds deduction is introduced.
- Maintain the 40-response design roster and separate authored recipe tier from element count. Mixed ground/air recipes, stock-backed selection and campaign factory/territory dispatch wiring remain future work.

### Validation, extender impact and next acceptance

- Re-run installed-asset/configuration validation after integration: verified GUIDs, navigation, force profiles, prepared tiers/recipes, tactical fixture settings and disabled Invader admission. Check copied context synchronization, merge markers and whitespace.
- Earlier development included source-adapted offline tactical/lifecycle checks with mocked native dependencies and Workbench-only smoke-test helpers. These do not establish native compilation or gameplay correctness.
- Preserve Rowan's imported Economy/Phone implementation and verify shared prefab/component coexistence at source level. The combined build still needs a successful Workbench compile and fresh Chicken/Rowantrek Play tests, including setup, Bank balances, income, QRF delivery/search/waves and multiplayer behavior.
- Extenders must account for the experimental direct REAPER dependency, opt-in tactical attributes, default-disabled Invader admission, immutable recipe tier/package semantics and deferred/retained capacity. Operation state, tactical contacts and wave timers remain session-local.
- Detailed operational settings and acceptance steps are in [QRF_TACTICAL_COMMANDER.md](QRF_TACTICAL_COMMANDER.md); ground and air fixtures have their own guides. No merge into `master-dev` or `master` is performed by publishing the development branch/PR.

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
