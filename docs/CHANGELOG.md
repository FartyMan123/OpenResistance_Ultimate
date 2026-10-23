# Changelog

## Unreleased — Commanders and QRF foundation (2026-10-23)

### Integration with master-dev

- Preserve Rowan's core factions, forced Resistance spawning and setup-flow updates while retaining the commander/scheduler components.
- Use the shared `AROR_FactionRolesComponent` as the authority for commander faction bindings. Remove the duplicate commander-folder role enum; the canonical definition lives under `Scripts/Game/Factions/`.
- Preserve Chicken's FIA/US/USSR QRF fixture with explicit shared-role and FIA loadout overrides. Original Everon keeps the core AROR faction keys.
- Extenders must move commander-manager faction attributes to `AROR_FactionRolesComponent`; evaluation interval remains on the commander manager. Recompile and reload prefabs/worlds before testing this integration.

### Added

- Three configurable role-based commander contexts: player-directed Resistance and AI-directed Occupiers and Invaders. Current bindings are FIA, US and USSR respectively.
- Server-owned attack and QRF scheduling, independent role cooldowns/capacity, target deduplication, bounded operation history and terminal cleanup.
- Weighted force pools and response levels 1–5. Each level deploys that many complete packages, with the whole response reserved before spawning.
- Ground execution for motorized, mechanized and wheeled-armour packages, including queued crew/infantry spawning, boarding, deployment waypoints and individual package-loss handling.
- Hostile-damage reporting from explicitly assigned defenders, with faction, instigator and location validation.
- Optional bounded mounted QRF town searches using reachable road points and native AI perception.
- Deployment faction audits, continuing vehicle-faction validation and optional same-faction damage diagnostics.
- Separate Everon_Chicken test world with Provins defenders, navigation bindings, perception manager and configurable US/USSR force pools. The current fixture selects three US mechanized LAV packages.
- Workbench smoke-test helpers, installed-resource/configuration validation and setup/testing guides.

### Fixed

- Designer force-profile/response-level arrays being overwritten during initialization.
- Duplicate inherited commander/scheduler components caused by inconsistent component IDs.
- Recursive operation-event invocation, empty standalone defender affiliations, and the agent-added callback signature in source.
- Premature loss detection for groups still materializing through the engine queue.

### Validation and remaining work

- Configuration validation passes for 25 installed resource GUIDs, three navigation bindings, eight profiles and five response levels; staged whitespace checks pass.
- Observed Play tests confirm hostile-hit QRF dispatch, motorized spawning/boarding/activation, and all three level-3 LAV packages boarding and becoming ACTIVE. Jasper observed the infantry truck arrive; the latest logs also show vehicle movement.
- The latest recorded run still used older compiled scripts and reported the previous agent-added callback error. Current callback, town-search and faction-diagnostic changes require a fresh Workbench compile and Play test. Apparent LAV friendly fire is not yet confirmed or excluded.
- Airborne transport/gunship profiles remain disabled. Flight, landing and escort execution is researched but unimplemented.
- Dedicated-server/multiclient acceptance, full deployment/dismount/search behavior, durable persistence, territory outcomes and economy integration remain outstanding.

### Extender impact

- The shared `AROR_GameMode_Base.et` now inherits commander and operation-manager components. Extenders should override these inherited components rather than add duplicates, and keep role bindings consistent with their scenario factions and loadouts.
- Force profiles must match configured commander factions. Ground profiles require matching crew, infantry and vehicle resources; unsupported airborne profiles are excluded.
- Executor pools and test fixtures are configured in Everon_Chicken; inheriting the shared game-mode template alone does not enable this test executor.
- Operation state is server-owned and session-local. No persistent save schema or client operation UI is introduced.

Architecture and project guidance are recorded in `ARCHITECTURE.md` and `CLAUDE.md`, with synchronized copies in `AGENTS.md`.
