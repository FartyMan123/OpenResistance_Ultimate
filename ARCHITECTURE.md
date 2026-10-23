# Architecture

Tracks the architecture of Open Resistance: its systems, sub-systems, and how they fit together. Update this file whenever a system is added, removed, or changes shape.

Status tags: **Planned** (designed, not built) · **In progress** · **Implemented**.

## Mod structure

### AR-Open-Resistance (CORE MOD)

All the scripts and functionality, plus templates for base-game assets.

- Lives in this repo: `AROpenResistance/Open Resistance/` (addon ID `OpenResistance`, GUID `6AA282336705EED2`).
- Class prefix: `AROR_` for all core classes, enums, and globals (e.g. `AROR_LocationComponent`). Reserved for the core mod.
- Depends only on the base game (`ArmaReforger`, `58D0FB3206B6F859`). It must stay playable on its own.
- Provides:
  - every game mode system (see [Systems](#systems))
  - template prefabs and configs built from base-game assets (locations, spawn points, factions, loadouts, AI groups) that extender mods can inherit from or replace
  - the extension points that extender mods use

### Extender Mods (made by others)

Separate addons, maintained by others, that add extra content such as maps, weapons, vehicles, scripts, functions, missions etc.

- Each one lists the core mod (`6AA282336705EED2`) as a dependency in its `addon.gproj`.
- Extends the core by:
  - inheriting from core template prefabs and configs
  - supplying their own configs (factions, loadouts, location data for new maps, missions)
  - subclassing core `AROR_` classes, or using `modded class` where needed
- Uses its own class prefix, never `AROR_`, so names don't collide with the core or with other extenders.
- The core must never depend on, or reference, an extender mod.

```
                 ┌─────────────────────────┐
                 │  ArmaReforger (base)    │
                 └────────────▲────────────┘
                              │ depends on
                 ┌────────────┴────────────┐
                 │ AR-Open-Resistance      │
                 │ (CORE MOD)              │
                 │ systems + templates     │
                 └────────────▲────────────┘
                              │ depends on
        ┌─────────────────────┼─────────────────────┐
 ┌──────┴──────┐       ┌──────┴──────┐       ┌──────┴──────┐
 │ Extender:   │       │ Extender:   │       │ Extender:   │
 │ maps        │       │ weapons /   │       │ scripts /   │
 │             │       │ vehicles    │       │ missions    │
 └─────────────┘       └─────────────┘       └─────────────┘
```

## Systems

Add each system here as it is designed or built, using the template at the end of this section.

### Game Mode — In progress
Purpose: the game mode entity every map places; all core systems hang off it as components.

Key classes/prefabs/configs:
- `Prefabs/GameModes/AROR_GameMode_Base.et`: inherits the base-game `GameMode_Plain.et` (`SCR_BaseGameMode`). Core system components are added here.
- A custom game mode script class will be added when game mode logic is needed.

Server/client: standard `SCR_BaseGameMode` replication.

Extension points: extender mods can inherit `AROR_GameMode_Base.et` to add or swap components.

Sub-systems (Planned):
- Game mode rules for the Resistance gameplay loop (to be defined).
- A playable world also needs faction manager, loadout manager and spawn point entities; these will be part of the map template.

### Locations — In progress
Purpose: named areas on the map that other systems use to decide where things happen (start location, AI, objectives). World-placed types: **Town, City, Airport, Factory, Military Base, Port, Resources**; spawned at runtime: **HQ**.

Key classes/prefabs/configs:
- `AROR_LocationComponent`: on each placed location. Name, type and a per-location radius set in the World Editor.
- `AROR_LocationManagerComponent`: on the game mode. Registry of all locations; other systems query locations through it.
- `AROR_ELocationType`: location types (`TOWN`, `HQ`, `AIRPORT`, `CITY`, `FACTORY`, `MILITARY_BASE`, `PORT`, `RESOURCES`); new values are appended so existing ones stay stable.
- `Prefabs/Locations/AROR_Location_Base.et` → one template per world-placed type (`AROR_Location_Town.et`, `_City`, `_Airport`, `_Factory`, `_MilitaryBase`, `_Port`, `_Resources`), each setting only its type and map icon. `AROR_Location_HQ.et`: a base-game spawn point entity with the location component (type HQ) and Persistence.
- Map icons (planned): each location prefab names its own map icon (image in our addon); `AROR_LocationMapIconsComponent` on the game mode draws them (Map Markers layer 1).

Server/client: towns are static world entities and HQs are replicated dynamic entities; both register on server and clients. Location state (control status, active/inactive) will be server-owned and replicated when added.

Depends on: Map Templates (for placement).

Extension points:
- New location types via `modded enum AROR_ELocationType` plus a prefab inheriting `AROR_Location_Base.et`.
- Extender maps place `AROR_Location_*` prefabs in their own sub-scene.

Sub-systems (Planned):
- Town state (e.g. who controls the town).
- Optional hand-placed child points (spawn, garrison, objective); fall back to scanning buildings inside the radius.
- Default name from the nearest base-game map descriptor.
- Workbench plugin to auto-place towns on a terrain.

### Game Flow — In progress
Purpose: the lifecycle from world boot to play, separating **scenario data** (from the world, never saved) from **playthrough data** (from a save).

Phases (mapped onto the base-game game mode states PREGAME → GAME → POSTGAME):
1. **Scenario load**: world boots, locations register, base location markers appear. No save involved.
2. **Setup** (PREGAME): the **server admin** sees the setup screen. Options: create a new playthrough (custom settings + pick the start location on the map) or load an existing one. Non-admin players see a "waiting for admin" screen. A server config fallback (auto-load latest / named save / new with defaults) lets dedicated servers start unattended.
   - **Setup UI** is built from base-game **configurable dialogs** (`SCR_ConfigurableDialogUi`) using our own dialog presets config, content layouts and handler classes. No base-game config (e.g. the menu registry `chimeraMenus.conf`) is overridden, so it cannot conflict with game updates or other mods. `AROR_SetupUIComponent` (client, on the game mode) drives it: "New" opens the base-game map menu, the admin clicks the HQ position and a dialog confirms it (validity checked client-side, re-checked by the server); "Load" steps through the saves sent by the server. Presets live in `Configs/UI/AROR_SetupDialogs.conf`.
3. **Playthrough load**: new → generate fresh state from settings + start location; load → restore state from the save. State markers are drawn. Transition to GAME.
4. **Running**: state changes are autosaved (see Persistence).

Pre-game is held by `AROR_GameFlowComponent`, our pre-game state component on the game mode (the base game skips PREGAME unless such a component exists); it allows the move to GAME only once a playthrough has been created or loaded, and keeps player controls disabled until then.

**Staging spawn** (Antistasi-style): each map places one `AROR_StagingSpawn.et` (spawn point + `AROR_StagingSpawnComponent`). While no playthrough exists, players spawn there as the Resistance and can move freely; the admin picks the HQ from there. Once a playthrough exists (new or loaded) the staging spawn is disabled; on a new playthrough every player is teleported to the HQ (keeping their gear).

**Resistance HQ**: the start choice. Placement rules (checked on the client for feedback, enforced by the server): on land, and at least a configurable distance (default 100 m) outside the radius of every location. On "New", the admin picks a spot on the map and the server spawns the Resistance HQ there. Players always spawn at the HQ. The HQ is a **dynamic location** (location type `HQ`): a spawned entity that carries the location component and the Resistance spawn point, so it shows up through the location registry and markers like any town, and is saved through entity persistence (*Register Dynamic*).

Settings:
- Chosen at setup, stored in the save, and **editable mid-playthrough** by the admin.
- Each setting is flagged *applies immediately* or *applies at the next cycle*; systems listen for a settings-changed event.
- Defaults and presets come from a core config that server admins and extender mods can override.

Server/client: the server owns phase transitions, settings and all generated state. Admin actions are client → server RPCs checked against base-game admin roles.

Depends on: Game Mode, Locations, Persistence.

Extension points: systems hook into lifecycle events (scenario ready, playthrough created, playthrough loaded, settings changed).

### Persistence — Planned
Purpose: unlimited, persistent playthrough saves.

- Built on the **base-game save system** (`SaveGameManager` plus entity persistence), not the community Enfusion Database/Persistence Framework mods, so the core mod depends only on the base game.
- Base-game model: a **playthrough** (one campaign) contains numbered **save points** (manual, auto, scripted, shutdown). Saves belong to a **mission**, so each map needs a mission header and its saves are kept separate automatically. Version compatibility checks come built in.
- **Limits**: the engine caps playthroughs and save points at about 101 each per mission, so old auto-saves need pruning. Saves carry no custom metadata, so a settings summary in the save list would need our own small index.
- **Boot rule**: if a save is active when the world loads, restore it and go straight to GAME (to verify); otherwise enter setup. Starting a new playthrough happens in the running world, with no reload (verified). It writes an initial save point straight away, so new and loaded games share one restore path.
- **Saving must be enabled for the scenario**: running the bare world in Workbench reports saving disabled. It is expected to come from the mission header (Save Types) and/or the world systems config (under investigation).
- **Where our data lives: inside the base-game save, via entity persistence. No separate save files.** Entities with the engine `Persistence` component are saved; the game mode already has it.
  - **Playthrough-wide state** (settings, start location, etc.) lives in our components on the game mode.
  - **Per-location state** (e.g. who controls a town) lives on the location entities, which get a `Persistence` component when they need it.
  - Each `AROR_` component with saved state has a matching serializer (`ScriptedComponentSerializer`, as base-game components do). Serializers write a version number first, so older saves keep loading after the data changes.
- Locations are referenced by a **stable location ID**, never by list position or entity reference. Saved data for a location that no longer exists is skipped with a warning.
- **Each system owns its save section** via three hooks: initialise fresh / restore from save / write to save. Extender mods add saved data the same way.
- Save **generated results**, not random seeds, so saves survive map and code changes.
- Server-only writes: periodic autosave, on key events (e.g. town captured, settings changed), and on shutdown.

### Factions — In progress
Purpose: the sides in play, referenced by **role** so the actual factions are configurable.

Two layers:
- **Roles** (gameplay): `AROR_EFactionRole` = `RESISTANCE`, `OCCUPIERS`, `INVADERS`. Code only ever refers to roles, never to faction keys.
- **Factions** (data): identity (key, name, colours, side), relationships and entity catalogs (which characters, groups and vehicles belong to it). The side and the models are independent, so any side can use any army's assets.
- Players always join the Resistance role's faction; there is no faction choice.
- `AROR_FactionRolesComponent` (on the game mode) maps role → faction key. Defaults: Resistance → **`AROR_RESISTANCE`**, Occupiers → **`AROR_OCCUPIERS`**, Invaders → **`AROR_INVADERS`**. Extender mods can point a role at their own faction without touching the core factions or code.

Core factions (in `AROR_FactionManager.et`), all mutually hostile, only the Resistance playable (base-game US, USSR, FIA, CIV stay present but not playable):

| Faction | Side | Default assets |
|---|---|---|
| `AROR_RESISTANCE` | INDFOR | FIA catalogs (FIA as faction ancestor) |
| `AROR_OCCUPIERS` | BLUFOR | US catalogs (US as faction ancestor) |
| `AROR_INVADERS` | REDFOR | USSR catalogs (USSR as faction ancestor) |

- `Prefabs/Managers/AROR_FactionManager.et` (inherits the base-game `FactionManager_Editor.et`: US, USSR, FIA, CIV) and `AROR_LoadoutManager.et` (inherits the empty `LoadoutManager_Base.et`; holds only the Resistance loadout) are placed in every map as part of the map template.
- `Prefabs/Characters/AROR_Character_Resistance_Base.et`: the Resistance player character (inherits the base-game FIA rifleman, which carries a map and compass).
- Resistance loadouts: the Resistance role's loadouts are the only ones offered; each includes a map and compass.
- Extension points: extender mods or server admins swap factions by overriding the config.

### Commanders — In progress
Purpose: one server-owned command context for each campaign role. Resistance is player-directed; Occupiers and Invaders are independently AI-directed, using the same evaluation interface.

Key classes/prefabs/configs:
- `AROR_Commander`: role identity, configured faction key and player/AI control mode.
- `AROR_CommanderManagerComponent`: on `AROR_GameMode_Base.et`; resolves faction bindings from the same entity's `AROR_FactionRolesComponent`, owns the three contexts and periodically offers the two enemy commanders for strategic evaluation after campaign setup permits play.
- `AROR_EFactionRole`: shared role identity for future military and territory systems.

Server/client: contexts and evaluation run only on the server. No commander UI, player-order RPCs or replicated strategic state yet. Runtime contexts are rebuilt from scenario configuration and contain no mutable campaign state to save at this stage.

Depends on: Game Mode, Factions and Game Flow. Readiness temporarily uses Game Flow's existing transition gate; campaign lifecycle events should replace this adapter when available. This inherits Game Flow's current save/restore readiness limitations.

Extension points: inherited game-mode prefabs configure faction bindings on `AROR_FactionRolesComponent` and evaluation cadence on the commander manager. Server systems subscribe to the manager's typed evaluation event and receive the commander whose turn is being evaluated. The Operations system consumes this event for optional configured attack targets; autonomous target selection remains planned. Each role is evaluated independently; Resistance never receives automatic evaluations.

Sub-systems (Planned):
- Player-directed Resistance orders with server validation.
- Enemy target selection and distinct Occupier/Invader strategy policies.
- Garrisons and military resources; Operations provides attack/QRF scheduling, weighted force selection and optional ground execution requiring runtime verification.
- Versioned persistence and client summaries when mutable strategic state is introduced.

### Operations — In progress
Purpose: schedule enemy attacks and defensive QRF responses independently for Occupiers and Invaders, tracking each request through execution and a terminal outcome.

Key classes/prefabs/configs:
- `AROR_OperationManagerComponent`: on `AROR_GameMode_Base.et`; subscribes to commander evaluations, schedules optional configured location targets, and accepts trusted server attack reports for QRFs.
- `AROR_QRFDefenseComponent`, `AROR_QRFDamageObserver`: optional server damage monitoring on explicitly assigned defending groups, characters or vehicles. Validates defender faction, hostile instigator and location bounds, then reports to the scheduler. Scenario/garrison code supplies and clears assignments; this does not establish territory ownership. An opt-in scenario initializer can assign an initial faction once to an unassigned standalone defender before monitoring starts; it does not override existing affiliations or initialize groups.
- `AROR_Operation`, `AROR_EOperationType`, `AROR_EOperationState`: role-owned requests referencing stable location IDs; lifecycle is planned → spawning → active → succeeded/failed/cancelled.
- `AROR_ForceProfile`, `AROR_ForcePool`, `AROR_EForceType`: designer-authored role/faction pools with separate attack/QRF weights, eligibility/capacity filtering and one-time profile binding per operation. Unsupported flight policies are excluded from selection.
- `AROR_ResponseLevel`, `AROR_ResponseScale`: configurable response levels 1–5, each deploying that many complete force packages. Level weights are separate for attacks and QRFs; feasible level selection precedes weighted force-type selection so additional profile variants do not bias strength probabilities. Profile and level remain bound to the operation.
- `AROR_VehicleTownSearch`: optional server coordinator for a bounded mounted QRF sweep using reachable road points inside the target location, followed by the package's normal deployment orders. It uses no player-position tracking and leaves target acquisition/combat to native AI perception. Attacks and infantry-only responses keep their existing deployment policy; runtime verification remains outstanding.

Server/client: all requests, scheduling and outcomes are server-only. No player RPCs or replicated operation UI. Each role has independent attack and QRF cooldowns and capacity; planned requests reserve capacity, with QRFs dispatched before attacks. Outstanding requests against the same target for one role are deduplicated. Timeouts and missing locations terminate requests, and retained history is bounded.

Depends on: Commanders, Game Flow readiness and the existing Locations registry. Territory/combat callers must establish target hostility or defensive ownership before submitting requests; Operations does not create a second ownership registry. Configured test attack targets bypass automatic target selection and must be appropriate for the attacking role.

Extension points: scenario prefabs configure schedules, limits, optional attack targets and force-profile arrays. Trusted server callers schedule attacks and report attacks against a defending role. `AROR_OperationExecutorComponent` provides optional server infantry and ground-vehicle execution, shared group/personnel/vehicle reservations, engine-queued members, activation acknowledgement and terminal cleanup. `AROR_GroundOperationForce` tracks separate crew/passenger groups, vehicles and their orders: motorized deliveries leave the vehicle at the dismount point, mechanized deliveries issue separate vehicle support orders, and armour remains mounted. Arrival does not change ownership or declare success. An empty pool retains the earlier infantry resource fields for compatibility.

Everon_Chicken inherits the same shared AROR role mappings and Resistance loadout as original Everon. Profiles distinguish the campaign faction from a declared native asset faction, allowing newly owned US/USSR assets to adopt their role faction once during spawning; unrelated affiliations and subsequent changes remain validation errors. Empty standalone defenders can resolve their initial faction from the shared defending role. It contains prepared US/USSR motorized, mechanized, wheeled-armour and airborne profiles using verified installed base-game resources. Airborne profiles include transport and gunship resources but stay disabled/excluded until a flight/landing/escort adapter exists. The original Everon world and shared game-mode prefab do not receive this test pool. Changing a role's faction requires matching profile configuration.

Multi-package execution reserves the complete response before member spawning. Packages deploy from separated launch positions and receive orders once all remaining packages are ready; individual combat losses degrade the response without replacing packages or failing surviving elements. Terminal cancellation, failure or timeout cleans up the whole operation. Ground route/convoy coordination is not automatic.

Persistence: operation IDs, queues, cooldowns and outcomes are currently session-local and are not saved. Restarting discards them. Native versioned persistence must be added before operations are treated as durable campaign state.

Sub-systems (Planned):
- Infantry and ground-vehicle execution are in progress and require Workbench verification. Launch positions/routes are designer-selected; automatic route/LZ validation, airborne flight and force handoff after success remain planned. Terminal cleanup retires operation-owned soldiers, corpses, vehicles and waypoints, preserving player-controlled entities/occupied vehicles. Engine-dormant groups retain their operation and reserved budget rather than counting as defeated.
- Hostile-damage reporting on explicitly assigned defenders is implemented pending Workbench verification. Authoritative ownership checks, capture invalidation, wider threat detection and restored garrison assignments remain responsibilities of combat/territory integration.
- Autonomous target selection and military-resource spending; executor capacity reservations exist, while economy costs and strategic availability remain planned.
- Save/restore and client summaries.

Development validation: Everon_Chicken has an `AROR_OperationTestComponent` for one-shot simulated QRF reports or scheduler/force-pool/response-scale smoke tests in Workbench. The current combat fixture places explicitly assigned US defenders in Provins, disables simulated reports and uses explicit profile/strength overrides for repeatable tests; clearing the executor overrides restores randomized supported profiles and levels. Chicken places the base-game Everon AIWorld prefab with explicit soldier, vehicle and low-resolution navigation bindings to installed Everon resources. The bindings use Bohemia's sample-world structure and require a fresh runtime check, because the prefab alone initialized empty navigation worlds in the earlier test. General test-component defaults remain inert; its simulated/smoke-test execution cannot run in packaged builds. Live logs verify hostile-damage reporting, QRF dispatch, motorized profile selection, spawning five personnel, boarding and activation. Driving, dismount and complete response execution remain unverified. The placed defense component does not establish territory ownership. No test executor is added to the original Everon world or shared game-mode prefab.

Airborne investigation: inspected base-game movement/controller/transport-waypoint interfaces do not establish an end-to-end landing/escort executor. A mission waypoint coordinator and verified flight adapter are still required; ordinary ground waypoints and cinematic movement are insufficient evidence. External helicopter-mod integration, if pursued, belongs in an optional extender so the core retains its base-game-only dependency. Findings and a proposed takeoff/transit/landing/unload/escort/return sequence are documented in `docs/AIR_OPERATIONS_RESEARCH.md`.

### Map Markers — In progress
Two layers:
1. **Location markers** (scenario load): **our own icon widgets** drawn on any open map (player map, deploy screen, setup screen) by a client-only component that follows the base-game map events (open/close/pan/zoom) and positions icons from world coordinates. Each location prefab chooses its icon image (white on transparent, so it can be tinted). No base-game config overrides, not draggable, no replication or saving. The HQ also keeps its spawn point map descriptor for the deploy screen.
2. **State** (playthrough load): location state is shown on the **same icon**, e.g. a town icon tinted by its controlling role (the faction's own colour, so it follows the Factions config). The control state is server-owned, replicated and saved on the location; each client re-tints its icon when it changes. Icons can also handle hover and click (used by the setup screen).

### Map Templates — Planned
Purpose: per-map setup of the game mode, as a sub-scene of the terrain's world.

- Core ships a template layout: game mode entity (with `AROR_LocationManagerComponent`), starting bases, and a `Locations` layer.
- One sub-scene per map with its own mission header. First map: **Everon** (`Worlds/AROR_Everon/`).
- Extender map mods create a sub-scene of their own terrain and place location prefabs; no code needed.

<!--
### System name — Status
Purpose: one or two sentences.
Key classes/prefabs/configs: `AROR_...`, `Prefabs/...`, `Configs/...`
Server/client: what runs where and what is replicated.
Depends on: other systems.
Extension points: what extender mods can override or add.
Sub-systems:
- ...
-->
