# Architecture

Tracks the architecture of Open Resistance: its systems, sub-systems, and how they fit together. Update this file whenever a system is added, removed, or changes shape. Keep it high level; implementation detail, test results and tuning notes belong in code comments, commit messages and `docs/`.

Status tags: **Planned** (designed, not built) · **In progress** · **Implemented**.

## Mod structure

### AR-Open-Resistance (CORE MOD)

All the scripts and functionality, plus templates for base-game assets.

- Lives in this repo: `AROpenResistance/Open Resistance/` (addon ID `OpenResistance`, GUID `6AA282336705EED2`).
- Class prefix: `AROR_` for all core classes, enums, and globals (e.g. `AROR_LocationComponent`). Reserved for the core mod.
- Intended to depend only on the base game (`ArmaReforger`, `58D0FB3206B6F859`). **Current exception:** it also depends on the REAPER helicopter mod (`69A4D664A6284E06`) for the airborne QRF experiment. This must be moved out (e.g. into `OptionalAddons/OpenResistanceREAPER`) before a base-game-only release.
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

### Optional addons (in this repo)

Extenders we maintain ourselves live in `OptionalAddons/`, each opened in Workbench through its launcher in `tools/` (which resolves the repo core and the game's downloaded addons without registering them globally).

- **Open Resistance - WCS** (`OptionalAddons/OpenResistanceWCS`, addon ID `OpenResistanceWCS`) — test project only. Holds no content; it depends on the core and the Worst Case Scenario (WCS) content addons, including the WCS RHS addons (so RHS Status Quo and WCS MRZR come too), so Workbench can load them together. WCS reaches the game mode through catalog augmentation (see Factions), not through this project; servers just load WCS. WCS's licence forbids modifying or adapting its content, so nothing here may inherit from or override WCS resources. Detail: `OptionalAddons/OpenResistanceWCS/README.md`.

### Base-game config overrides

The core adds entries to two base-game configs with **Override in addon**, which stores only our additions on top of the base file: `Configs/System/chimeraMenus.conf` (the phone menu) and `Configs/System/chimeraInputCommon.conf` (the phone keybind). Everything else uses our own configs.

### UI design system

All of our UI (HUD, notifications, map popups, phone apps) follows the Open Resistance UI design system, summarised in [docs/UI_DESIGN.md](docs/UI_DESIGN.md): Roboto fonts from the base game, colour tokens in `AROR_UIColors`, and one name per thing (Readiness, Crackdown, REBINTEL, HICOMM). Colours come only from `AROR_UIColors`.

## Systems

Add each system here as it is designed or built, using the template at the end of this section.

### Game Mode — In progress
Purpose: the game mode entity every map places; all core systems hang off it as components.

- `Prefabs/GameModes/AROR_GameMode_Base.et` inherits the base-game `GameMode_Plain.et` (`SCR_BaseGameMode`); core system components are added to it.
- Server/client: standard `SCR_BaseGameMode` replication.
- Extension points: extender mods inherit `AROR_GameMode_Base.et` to add, configure or swap components.

### Locations — In progress
Purpose: named areas on the map that other systems use to decide where things happen. World-placed types: **Town, City, Airport, Factory, Military Base, Port, Resources, Outpost**; spawned at runtime: **HQ**.

- `AROR_LocationComponent` on each location: stable ID, name, type, radius, map icon and initial controlling role.
- `AROR_LocationManagerComponent` on the game mode: registry of all locations, with registered / unregistered / control-changed / support-changed events. Other systems query locations through it.
- Templates: `Prefabs/Locations/AROR_Location_Base.et` and one prefab per type; the military types carry a capture flag.
- **Control**: every location is always controlled by a role (Resistance, Occupiers or Invaders). Server-owned, replicated, saved per location (`AROR_LocationComponentSerializer`). The Resistance HQ is fixed to the Resistance.
- Server/client: placed locations are static world entities and the HQ is a replicated dynamic entity; both register on server and clients.
- Saved: controlling role, referenced by stable location ID.
- Depends on: Factions, Map Templates (placement).
- Extension points: new types via `modded enum AROR_ELocationType` (values are appended so existing ones stay stable) plus a prefab inheriting `AROR_Location_Base.et`; extender maps place location prefabs in their own sub-scene.
- Planned: hand-placed child points (spawn, garrison, objective); default names from base-game map descriptors.
- Everon campaign data: a catalogue of 41 stable locations (18 settlements, 23 military installations) generated into the Chicken world layers by scripts in `tools/`, plus hand-placed seaports (Lamentin, Saint-Philippe, Morton) and the Invader stronghold. See `docs/EVERON_CAMPAIGN_LOCATIONS.md`.

### Strongholds — In progress
Purpose: a faction's home base, built and defended so that the Resistance cannot take it with an early rush. Chicken has two: **Fortress Vostok**, the Invaders' starting base at Saint-Pierre harbour, and **Stronghold Delaware**, the Occupiers' home base at Everon International Airport.

- `AROR_StrongholdComponent` on the stronghold's location entity. Vostok is a `MILITARY_BASE` with a level-4 garrison (`AROR_Defence_Fortress_Vostok.et`, generated by `tools/Set-FortressVostok.ps1`). Delaware extends the airbase compound with an outer wall, extra guns and teams, and three unmanned helicopters on pads (`AROR_Defence_Stronghold_Delaware.et`, generated by `tools/Set-StrongholdDelaware.ps1`).
- Only its home role or the Resistance can take it (`AROR_LocationComponent.CanBeTakenBy` asks the component).
- **Retaliation**: during an early-game window (campaign time with players online), defenders killed by players are counted; at the threshold the home role sends a scripted level-5 response in two waves, and the count restarts.
- **Final defence**: after the window, a Resistance assault (a player inside the radius or a player kill) draws a scripted level-5 combined-arms response, re-armed after a cooldown once it ends.
- Scripted responses launch from the home role's best friendly base when the stronghold allows it (Delaware), otherwise from the coast (authored landing points, else a bounded coastal search), and run through Operations with funding waived (see Operations).
- Server/client: all logic is server-only; clients only see the replicated location and the headlines.
- Saved: early-window progress and the player kills counted towards the next retaliation (`AROR_StrongholdComponentSerializer`). Live responses are session-only.
- Depends on: Locations, Location Capture, Garrisons, Operations, Notifications.
- Extension points: every threshold, recipe, level and landing setting is an attribute on the component; extender maps place the composition (or their own) with the component and an AI home role.
- Not built: a campaign end when a stronghold falls. Feature detail: `docs/STRONGHOLDS.md`.

### Location Capture — In progress
Purpose: how a location's controlling role changes. All changes are made by the server through the location's control; the HQ cannot be captured.

- **Towns & Cities**: held by the role with **more than 50% support** (see Settlements).
- **Military locations** (airport, factory, military base, port, resources, outpost): captured at the location's **capture flag** by interaction, only when **every hostile** inside the radius is dead.
- **Settlements are support-only**: towns and cities reject flag capture and AI recapture; they change hands only through support (see Settlements).
- **Flip cooldown**: after a Resistance capture, the location cannot flip back to the Occupiers or Invaders for **5 minutes**. Replicated, not saved.
- **AI recapture** (`AROR_RecaptureComponent`, server): every few seconds the forces inside each capturable location are weighed; defenders count ×2 and attacking AI ×1. An attacking AI role that outweighs the defenders takes the location, subject to the cooldown.
- **Aircraft occupants never count**: characters seated in a helicopter are ignored by both the flag's hostile check and AI recapture, so a passing or landing aircraft cannot block or flip a capture.
- **Capture flag placement**: optionally, the server moves a capture flag to a clear, walkable spot near the location centre when play starts ("Auto Place"), and replicates the result.
- Key classes: `AROR_CaptureLocationAction` (user action on `AROR_Prop_CaptureFlag.et`), `AROR_RecaptureComponent`, `AROR_CaptureFlagPlacementComponent`.
- Server/client: clients check the rules only to show feedback; the server re-checks and decides.
- Depends on: Locations, Factions, Settlements. Operations reacts to capture events without changing these rules.

### Settlements — In progress
Purpose: towns and cities as places with people: population, and how much each role is backed there (support), which decides who holds them.

- `AROR_SettlementComponent` on town and city prefabs: **population** (set per placed location) and **support** per role, which always sums to 100%.
- A new campaign starts at 100% for the initial controlling role. When another role goes **above 50%**, control flips to it.
- Server-owned, replicated, raises a support-changed event, saved per location (`AROR_SettlementComponentSerializer`).
- **Regional support** (`AROR_RegionalSupportComponent`, optional scenario policy, off by default): the server gradually moves each settlement's support toward the share of nearby military installations each role holds. Uses the existing support and ownership authority; nothing extra is saved. Otherwise support changes only through the server API and admin tools.
- Depends on: Locations, Factions.

### Garrisons — In progress
Purpose: defend flag-capturable military installations with a saved roster whose physical defenders spawn when players are near. Towns, cities and HQs are excluded.

- `AROR_GarrisonComponent` (on military location templates) owns the roster, losses and placement results; `AROR_GarrisonManagerComponent` (game mode) activates and retires defenders by player proximity and paces spawning. `AROR_GarrisonFaction` binds each role to the infantry, crew and vehicle assets it uses.
- Defence layouts (`AROR_DefenceLayout`, `AROR_DefencePositionComponent` and the `AROR_Defence_*` compositions) define where defenders and vehicles stand. Static-gun markers may be raised onto towers or roofs, guard towers tagged as sentry posts each hold a marksman (his own group, saved through the infantry variant), tagged helipads hold parked helicopters on their decks, and a garrison may declare a rectangular footprint for a narrow site. Editor tools audit and ground these sites.
- Garrison catalogues can provide static weapon variants; authored recipes can override their count, add infantry teams and add unmanned parked vehicles such as helicopters (`m_iExtraInfantryTeams`, `m_aExtraParkedVehicles`). Logical variants and losses use the existing roster save data. Optional equipped compositions carry weapon, ammunition, equipment and medical stores for Resistance looting. `AROR_OutpostSupplyComponent` stocks each store once from the roster army's faction preset when the roster is first generated (a saved roster never restocks), then the native inventory owns the items and their replication. The current store props are decorative and cannot hold items; they need replacing with an openable container. Native stock/depletion and inventory save/load acceptance remain pending.
- Server/client: roster, spawning and saves are server-owned; clients see the replicated defenders. A replicated gate stops capture while a defence is still deploying.
- Saved: the roster (what remains and what was lost) per location, through Persistence. Defenders are not refilled when players leave and return.
- Depends on: Locations, Location Capture, Factions, Operations (damage reporting).
- Role asset bindings can use the role's faction preset: its static MGs, armed cars and APCs are appended to the static and manned choices (after the authored ones, so saved variants keep their meaning), and a role given another army's preset takes its soldiers and assets from that preset. Both core bindings opt in.
- Extension points: scenario and editor settings (activation range, capacity, defence levels, layouts); role asset catalogues can be replaced for extender factions.
- Planned: preset-driven units and vehicles (see Factions), restoring exact inventory and live positions, replenishment.
- Feature detail: `docs/GARRISONS_AND_REGIONAL_SUPPORT.md`, `docs/MILITARY_DEFENCE_PRESETS.md`, `docs/FORTIFIED_OUTPOSTS.md`.

### Game Flow — In progress
Purpose: the lifecycle from world boot to play, separating **scenario data** (from the world, never saved) from **playthrough data** (from a save).

Phases (mapped onto the base-game states PREGAME → GAME → POSTGAME):
1. **Scenario load**: world boots, locations register, location markers appear.
2. **Setup** (PREGAME): the server admin creates a new playthrough (choosing a faction preset per role, or a quick start with the base-game presets, then picking the Resistance HQ position on the map) or loads a saved one. A server config fallback for unattended dedicated servers is planned.
3. **Playthrough load**: new → fresh state; load → restore from the save. Then GAME.
4. **Running**: state changes are saved (see Persistence).

- `AROR_GameFlowComponent` (server, game mode) holds PREGAME until a playthrough exists and keeps player controls disabled until then; other systems can ask whether the playthrough is ready.
- **Setup UI** (`AROR_SetupUIComponent`, client): a full-screen setup menu (`AROR_SetupMenu`, registered in `chimeraMenus.conf`) with the start options (new, load, quick start) and the save list; the faction and HQ steps use base-game configurable dialogs with our own presets (`Configs/UI/AROR_SetupDialogs.conf`).
- **Staging spawn** (`AROR_StagingSpawn.et`): before a playthrough exists, players spawn there as the Resistance and move freely. Afterwards it is disabled and players are moved to the HQ.
- **Resistance HQ**: placed on land, outside every location's radius, and on a clear 10×10 m pad free of buildings and props (checked on the client, enforced by the server; invalid picks are rejected). A dynamic location of type HQ that carries the Resistance spawn point; players always spawn there. A new playthrough also spawns an arsenal beside the HQ via `SpawnArsenal` and a garage on the opposite side of the flag (same offset negated) via `SpawnGarage` before the initial save.
- Planned: playthrough settings chosen at setup and editable mid-game, with lifecycle events (scenario ready, playthrough created/loaded, settings changed) for other systems.
- Server/client: the server owns phase transitions and all generated state; admin actions are client → server requests checked against admin roles.
- Depends on: Game Mode, Locations, Persistence.

### Persistence — In progress
Purpose: persistent campaign saves, built on the **base-game save system** (`SaveGameManager` plus entity persistence), so the core needs no persistence mods.

- A **playthrough** (one campaign) holds numbered save points; saves belong to a mission, so each map needs a mission header. Saving must be enabled for the scenario (Workbench Play on the bare world has saving disabled).
- **Our data lives inside the base-game save, no separate files.** Entities with a `Persistence` component are saved; the game mode and locations have one.
  - Playthrough-wide state lives in our components on the game mode; per-location state lives on the location entities.
  - Each component with saved state has a matching `ScriptedComponentSerializer`. Serializers write a version first and keep loading older versions.
  - Locations are referenced by **stable ID**, never by position in a list.
- Save generated results, not random seeds, so saves survive map and code changes.
- Saved so far: location control, settlement support, the Resistance HQ, arsenals, faction presets per role, faction funds, manpower, player money, Resistance item stock, Resistance garage vehicle snapshots, per-player arsenal loadout slots, garrison rosters, QRF readiness, crackdown and debt, the Invader campaign state, stronghold state and active Resistance missions.
- Boot rule: if a save is active when the world loads it is restored; otherwise setup starts. A new playthrough writes an initial save point, so new and loaded games share one restore path.
- Limits: about 101 playthroughs and save points per mission; old autosaves need pruning.
- Planned: periodic autosave and saves on key events and shutdown; verification on a dedicated server.

### Factions — In progress
Purpose: the sides in play, referenced by **role** so the actual factions are configurable.

Three layers:
- **Roles** (gameplay): `AROR_EFactionRole` = `RESISTANCE`, `OCCUPIERS`, `INVADERS`, `CIVILIANS`. Code refers to roles, never to faction keys or prefabs.
- **Factions** (identity): name, colour, flag and relationships. `AROR_FactionRolesComponent` (game mode) maps each role to a faction key: `AROR_RESISTANCE`, `AROR_OCCUPIERS`, `AROR_INVADERS`, `AROR_CIVILIANS`.
- **Faction presets** (equipment): what an army makes available to the game, independent of identity, so a role can keep its name and colour while using another army's equipment.

Core factions live in `AROR_FactionManager.et`. The three military factions are mutually hostile; Civilians are friendly to all. Only the Resistance is playable, and players always join it.

| Faction | Side | Default preset |
|---|---|---|
| `AROR_RESISTANCE` | INDFOR | `FIA` |
| `AROR_OCCUPIERS` | BLUFOR | `US_ARMY` |
| `AROR_INVADERS` | REDFOR | `USSR_ARMY` |
| `AROR_CIVILIANS` | civilian | `EVERON_CIV` |

**Faction presets — In progress**
- A preset (`AROR_FactionPreset`, one `.conf` each) lists what an army makes available in eight fixed categories: **Units** (AI characters, at most one per unit type), **Vehicles** (empty, no crew), **Gear**, **Equipment**, **Weapons**, **Weapon accessories**, **Ammunition** and **Vehicle ammunition** (munitions loaded onto vehicles and aircraft as items, such as helicopter rockets; vehicle-mounted guns rearm through the supply system instead). Every entry is a prefab with a **base cost** and a **minimum readiness** (default 1) when it becomes available, grouped by type (e.g. RIFLEMAN, MOTOR_TRUCK, RIFLE); apart from units, a type can have any number of entries.
- Presets only make things available; the systems that use them (garrisons, QRFs, shops, recruitment) decide what to pick. Each preset lists the roles it can be used for.
- `AROR_FactionPresetRegistry` (`Configs/FactionPresets/AROR_FactionPresetRegistry.conf`) lists the available presets. The base-game presets (`FIA`, `US_ARMY`, `USSR_ARMY`, `EVERON_CIV`) are built into the core, written the same way an extender would.
- `AROR_FactionPresetsComponent` (game mode) loads the presets on every machine and answers queries for a role's preset. The server owns which preset each role uses: chosen by the admin per role when creating a campaign (defaults otherwise), replicated, and saved (`AROR_FactionPresetsComponentSerializer`). A saved preset that no longer exists falls back to the role's default.
- **Catalog augmentation**: a preset can name a catalog faction (e.g. `US`). A frame after loading, on every machine, it adds everything that faction's entity catalogs hold and it does not list: units only for unit types it lacks, everything else with a default Readiness by type (heavier vehicles later). Listed entries always win, so hand tuning stays. A preset can also take hand-held weapons, accessories and ammunition from further factions, leaving out items from named addon filesystems. This is how content mods such as WCS, which add their assets to the base-game catalogs, reach the game mode with no extender: the base presets augment from `US`, `USSR`, `FIA` and `CIV`, and FIA takes the US/USSR hand-held items that are neither base-game nor RHS. Sorting rules (`AROR_CatalogClassifier`) are shared with the Workbench import of the base presets.
- QRF force profiles can pick their vehicle and air escort from the role's preset (see Operations), and garrison bindings can take their assets from it (see Garrisons).
- To verify: whether Override in addon merges additions into the registry's plain path list or replaces it (named entries are proven to merge, plain arrays are not). Needs a test with two addons; if it replaces, the registry switches to named entries.

- `AROR_LoadoutManager.et` offers only Resistance loadouts; `AROR_Character_Resistance_Base.et` is the player character.
- Extension points: extender mods add presets (new `.conf` files listed in the registry through Override in addon). The categories and their types are a fixed core list; entries with any other type are ignored. Server admins or extenders point a role at another faction key without touching core code. Mods that add assets to the base-game faction catalogs (e.g. WCS) need no extender at all, through catalog augmentation.

### Commanders — In progress
Purpose: one server-owned command context per role. The Resistance is player-directed; the Occupiers and Invaders are AI-directed through the same evaluation interface.

- `AROR_Commander` (role, faction, control mode) and `AROR_CommanderManagerComponent` (game mode), which periodically offers each AI commander for evaluation once the playthrough is ready.
- `AROR_InvaderCampaignComponent` (optional, game mode): the Invaders' push north from an offshore support corridor. On a fixed period they push on the southernmost military site they do not hold, within reach of their own sites. Against the Occupiers the battle is simulated and resource-funded (never near players; the push waits instead), and against the Resistance it is a physical attack through Operations. At their goal share they stop taking Occupier ground, and above it the Occupiers win back the outermost Invader site in the simulation, so the front settles around the goal. It spends Economy funds and changes control through the normal location authority. Chicken opts in.
- Server only; not replicated. Saved: the Invader campaign state (its own serializer).
- Depends on: Game Mode, Factions, Game Flow.
- Extension points: server systems subscribe to the evaluation event (Operations does).
- Planned: enemy target selection and distinct Occupier/Invader strategies.

### High Command — In progress
Purpose: Resistance Leaders raise AI squads and command them through the **HICOMM** phone app.

- `AROR_HighCommandComponent` (game mode): the Resistance Leaders (the host automatically; the host delegates others), the resistance name, a squad catalogue and the raised squads. Squads cost Resistance funds **and** manpower, form up at the HQ and take **attack**, **defend**, **move** (standing by on arrival) and **hold** orders. Each leader commands only their own squads; everyone sees all of them.
- Client: the HICOMM app (see Phone) sends requests through RPCs on the player controller.
- Server/client: everything is decided and validated on the server; a compact snapshot (leaders, name, squad summaries) replicates to clients.
- Saved: nothing. Squads are refunded at full cost when the campaign ends, and saved funds and manpower already include the refund for living squads.
- Depends on: Factions, Locations, Economy, Game Flow (the HQ), Phone, Notifications.
- Extension points: squad catalogue, per-leader cap, group/waypoint prefabs and map bounds on the component.
- Feature detail: `docs/HIGH_COMMAND.md`.
- Planned: more order types, platoons, enemy intel on the map, balance.

### Missions (OPS) — In progress
Purpose: Resistance side missions, offered on a board and run through the **OPS** phone app, paid in Resistance funds and manpower.

- `AROR_MissionBoardComponent` (game mode): keeps a board of mission offers (rerolled as offers expire or are taken) and the active missions. Only **Resistance Leaders** (see High Command) accept or abort them; active missions are capped. A mission's enemies and props spawn when a Resistance player comes near, and its leftovers are removed once players leave.
- Mission types (`AROR_EMissionType`), one `AROR_Mission` subclass each: **Supply Theft**, **Convoy Ambush**, **Prisoner Rescue**, **Supply Sabotage**, **Officer Assassination** and **Arms Cache Recovery**. Each picks its target from Locations (usually an enemy-held site), takes its units and vehicles from the enemy role's faction preset (vehicles limited to the army's current Readiness), and checks its own objectives and failure conditions. Rewards, time limits and guard counts are per type (`AROR_MissionType`) and rise with the target's threat level.
- Results: success pays the Resistance through Economy; start, success and failure are announced through Notifications; a short network log is kept for OPS.
- Server/client: everything is decided on the server; a compact snapshot (missions and log) replicates to every client. Clients request through RPCs on the player controller. Active missions are drawn on the game map (Map Markers), in OPS and in HICOMM.
- Saved: active missions (identity, target, rewards, time left) through `AROR_MissionBoardComponentSerializer`; a loaded mission restarts when Resistance players approach and can be aborted by any current Resistance Leader. Offers are not saved; the board rerolls.
- Depends on: Locations, Factions (presets), Economy, High Command (leaders), Notifications, Phone, Map Markers.
- Spawning (groups, vehicles, seating, waypoints, road positions) goes through `AROR_ForceSpawner`, shared with Patrols; the board keeps thin wrapper methods for missions.
- Extension points: per-type settings and board size, caps, ranges and prefabs on the component; new mission types through `modded enum AROR_EMissionType` plus an `AROR_Mission` subclass returned from a modded `CreateMission`.
- Feature detail: `docs/MISSIONS.md`.
- Planned: enemy crackdown and support effects, item rewards, more mission types, balance.

### Black Market — In progress
Purpose: a smugglers' shop in the phone where Resistance players buy vehicles and firearms, gated by the seaports the Resistance holds. It will feed the virtual garage and arsenal (in progress separately).

- `AROR_BlackMarket`: the catalogue (curated vehicles and firearms, each with a price, a port tier and a base-game prefab), access and ordering. No seaport held means no access ("NO ACCESS TERMINAL TO BLACK MARKET"); tiers are Dockside (1 port), Contraband (2) and Heavy Cargo (3+).
- Server/client: the client asks through the player controller; the server checks Resistance membership, ports and personal money, takes payment and records the order, then sends a hint and the player's order list back to them only. Port counts read replicated location control.
- Orders wait for a collection point: delivery is not built yet. `GetOnOrderPlaced_S` is the seam for the virtual garage / arsenal.
- Saved: nothing yet. Orders last for the session.
- Depends on: Locations (seaports), Economy (player money), Factions (Resistance check), Phone, Notifications.
- Extension points: modded `AROR_BlackMarket.BuildCatalogue` to change what is sold; `GetOnOrderPlaced_S` for delivery.
- Planned: delivery through the garage / arsenal at a held port, saved orders, Attention (black market heat).

### Operations (Attacks & QRF) — In progress
Purpose: enemy **attacks** and defensive **quick reaction forces (QRF)** for the Occupiers and Invaders: deciding when to respond, choosing and paying for a force, spawning and directing it, and cleaning up afterwards.

Main parts:
- **Scheduling** (`AROR_OperationManagerComponent`): accepts attack requests and threat reports, ranks threats, applies per-role cooldowns, capacity and deduplication, and tracks each operation from planned to a final outcome. Invader operations are **disabled by default** until territory and resource integration is ready.
- **Force selection**: designer-authored force profiles and ground recipes per role and faction (motorized, mechanized, armour, airborne), response levels 1–5, and an optional strategy layer: saved **Readiness** (was "war level") decides which equipment a response may use, and a separate saved **Crackdown** (was "aggression") sets its strength. Both are integer levels 1–5 per enemy role.
  A profile can pick its hull (and air escort) per dispatch from the role's faction preset: a random vehicle of a set type available at the operation's Readiness, so content mods reach QRFs through catalog augmentation. Ground crews grow to the hull's driver and turret seats and passengers are capped to the reserved personnel budget; a picked hull that cannot fit the package (or an aircraft without a pilot or passenger seat) is replaced by the profile's authored vehicle. Chicken enables this for all its profiles (armour picks IFVs, since its armour profiles carry infantry).
  New campaigns begin at Readiness 2 and response tier 2 (both scenario settings), and Crackdown does not cool during quiet periods (cooling is a setting, off by default). Capture losses build intensity in regular steps; authored ground/joint bands add one full package per tier while preserving equipment gates and separate infantry/vehicle crews. Loaded escalation remains saved campaign state.
  Ordinary campaign QRFs target a running airborne share per army, counting standalone and joint deployments together. Successful dispatch totals are saved; manual/debug/scripted responses are excluded. Equipment, finance and launch safety can force a ground fallback without consuming an airborne dispatch.
- **Threat labels** (`AROR_ThreatLabels`): display names for each level, keyed by the army (faction preset) holding the role, e.g. ГОТОВНОСТЬ/РЕЖИМ for the Soviet Army and THREATCON/CRACKDOWN for the US Army, with a generic English fallback. Logic only uses the integers.
- **Funding** (`AROR_QRFFinance`): responses reserve and spend the role's funds through the **Economy** API; losses are charged and shortfalls become saved QRF debt. Economy remains the only bank.
- **Execution** (`AROR_OperationExecutorComponent` and helpers): spawns and reserves forces, delivers them by road or by air, runs searches and town security, and staged reinforcement waves under a shared tactical commander (`AROR_QRFTacticalCommander`). Forces launch from friendly outposts, bases or airports.
  Ground vehicle dispatch requires an assigned living AI driver and crew in every turret seat, including mounted weapons attached to cargo-style compartments; passenger occupancy alone cannot satisfy combat readiness. Crew and passengers are teleported into their seats; the engine refuses this for AI it has not woken (packages spawned far from players), so members still unseated after the retries are replaced by the seat's default occupant spawned straight into the seat, in the same group.
  Road launches fall back to a road beside each probe when the base centre is cut off on the navmesh, and a base with no reachable road at all is dropped as a launch site for the session. Road approaches fall back to the nearest launch-reachable road within the dismount limit, with infantry walking in. The active-operation timeout runs from the estimated arrival.
  Packages left behind by a failed or ended operation are retained until players are far enough away. A package that never boarded does not count toward the live ground cap and is cleared once no player is within 300 m. Launch waits (live cap, missing commander, unusable bases by reason, missing recipe) are logged.
  Ground profiles can fill native passenger capacity with a separate infantry group within their reserved personnel budget. Chicken enables this policy for its ground vehicles: infantry unload at the delivery point while driver and turret crews retain the hull for transport or support. Personnel reservations and collateral cover the full-load ceiling before construction; actual seat capacity is checked before queuing actors.
- **Airborne execution** (experimental, `AROR_ReaperQRFExecutor`): helicopter insertion with optional gunship support using REAPER. Chicken provides light transport-only packages from Readiness 1 and escorted packages from Readiness 5. Support investigates reported positions through native perception, with traffic avoidance and separate cruise/support routes; airborne execution can be switched off.
  Chicken transport cabins use native usable passenger capacity within pre-reserved full-load ceilings. Transport and escort pilots remain separate from infantry; boarding targets distinct compartments and requires actual occupancy before the shared lead barrier releases flight. Initial admission includes finite landing-search, construction and boarding allowances.
- **Joint responses** (experimental, scenario opt-in): authored recipes combine one airborne package with ground elements under one operation and tactical commander. Chicken offers truck/light-support mixes at Readiness 2, mechanized mixes at 3–4, and escorted/armoured mixes at 5 for both armies. Funding, actor reservations, activation, outcome and cleanup are shared. Joint attacks hold airborne insertion for ground assault release; native acceptance is pending.
- **Territory attacks** (`AROR_TerritoryAttackManager`, optional scenario policy): the Occupiers attack Resistance-held military sites from the moment the Resistance holds any, front site nearest an Occupier base first, after a short per-site capture grace and with a rest between their attacks. A site's deployed capture QRF holds attacks on it, plus a recovery time after it ends. The Invaders' physical attacks come from their push (Commanders) through the same launch path. Attacks share QRF force selection, funding, tactical coordination and cleanup. Only one attack may be outstanding globally, whichever army; normal capture authority decides success. Settlements remain support-only, HQ is excluded, and strongholds are only attacked by an army that can capture them. The server logs why no attack is starting whenever that reason changes. In Workbench builds, MARS can launch any response as an attack (staging and assault) without the campaign timers; packaged servers refuse it, and test attacks never move the real attack timers or rest.
- **Response policy** (`AROR_ResponsePolicy`): a shared, side-effect-free selector uses Readiness for equipment availability and Crackdown for intensity. Response families resolve configured profiles or authored recipe tiers, with the same eligibility gates in campaign planning and MARS. Recipes can declare their response family; extenders can add families, profiles and tiers without adding phone dispatch logic.
- **MARS debugger** (Phone): role-separated response-type grid with simulated Readiness and Crackdown sliders, live force preview and explicit REAPER airborne packages, dispatched through the same server operation system to the nearest Resistance-owned territory. The server resolves and freezes the selected metrics. Debug dispatch requires an owned safe military origin, preserves actor limits and bypasses campaign finance without changing campaign Readiness or Crackdown. The client uses its own red layout and cached circular backdrop.
- **Scripted responses** (`AROR_ScriptedResponses.c`): trusted server callers (Strongholds) can request a QRF with a named recipe, Readiness and Crackdown frozen at a level, funding waived and a fixed launch point instead of a friendly site, optionally forced into two waves. It skips scheduler cooldowns and the per-role cap, but keeps target conflicts, Invader admission, executor capacity and launch safety; execution is otherwise a normal QRF. Coastal staging is not a friendly recovery base or simulated sea transport.
- **Defence and spotting**: assigned defenders report hostile damage; capture events can trigger a response for the previous owner. Resistance players who see QRF vehicles get last-seen map markers.
- **Dispatch alerts** (`AROR_OperationAlerts.c`): when a QRF or attack is dispatched, everyone gets a headline naming the army, the target and a rough arrival estimate (`AROR_OperationEta`, from each lead package's distance). A replicated board of outstanding QRFs and attacks drives a target marker on the threatened location (Map Markers).
- **Assault and security**: ground attacks stage before a shared assault release. Targeting uses recent attributed combat reports; hidden defenders are not revealed by a world-state census. A response physically holding a lost settlement can shift support through Settlements while outnumbering all hostile military forces there; support remains the settlement ownership authority.

- Server/client: all decisions and execution run on the server. Clients only receive QRF spotting reports (their own, through the player controller), send whether their map is open, and read a public readiness/crackdown summary (RebIntel) and the outstanding-response board (dispatch alerts).
- Saved: readiness, crackdown, QRF debt and aggregate ordinary-QRF dispatch/airborne counts per army. Live operations, deployed forces, queues, territory attack eligibility and cooldowns are session-only. Loaded territory establishes a fresh baseline: each held site becomes attackable once the capture grace has passed in the current session.
- Depends on: Commanders, Locations, Location Capture, Factions, Economy, Game Flow; the REAPER mod for airborne execution.
- Extension points: scenario and prefab configuration of schedules, limits, force profiles, recipes and prices; trusted server callers can request attacks and report threats; extender armies add Readiness/Crackdown labels with a modded `AROR_ThreatLabels.Create`.
- Feature detail: `docs/OPERATIONS.md`, `docs/QRF_STRATEGY_ECONOMY.md`, `docs/QRF_TACTICAL_COMMANDER.md`, `docs/AIRBORNE_QRF_PROTOTYPE.md`, `docs/QRF_SPOTTING.md` and the other QRF files in `docs/`. Testing in the Chicken world: `docs/OPERATION_EXECUTOR.md`, `docs/FORCE_POOL_QUICKSTART.md`, `docs/TRUCK_IFV_QRF_TEST.md`.
- Release validation and native acceptance: `docs/QRF_RELEASE_VALIDATION.md`; route and response matrices live in `docs/validation/`. The Workbench-only route audit probes ground planning, airborne launch formations and landing sites without deploying forces.
- Planned: broader strategic target selection, physical military stock, durable save/restore of live operations, client summaries.

### Patrols — In progress
Purpose: ambient enemy activity around the players, so fights happen between captures and QRFs: Occupier and Invader **foot patrols** and **motorized road patrols** that cross the areas where Resistance players are.

- `AROR_PatrolManagerComponent` (game mode, server): every few seconds, picks a living player's area that has fewer patrols than wanted (a base number, plus one per two Crackdown levels of the patrol's army) and spawns one, within global caps on patrols, soldiers and vehicles. Its soldier budget is separate from Garrisons and Operations.
- **Bounded to the players, fair to them**: a patrol spawns on a road out of every player's range, outside every military site and Resistance location. Its route is fixed at spawn: a road point near the area's centre, then a road exit on the far side. It never re-targets players.
- **One army at a time**: a patrol belongs to the army holding the nearest military site and must stay in that army's ground. Routes keep clear of Resistance military sites and the HQ (an enemy patrol must never wander in and recapture them) and well clear of the other army's sites, so the two armies do not meet.
- **Strength**: foot team size grows with Crackdown; motorized patrols use cars, armed cars from Readiness 3 and APCs from Readiness 4, from the army's faction preset, with passengers.
- **Robustness**: spawning and seating have timeouts that abandon and clean up the patrol instead of retrying; legs with no progress are skipped; patrols end after a lifetime, and despawn once no player has been near for a while and they have not fought recently. A wiped-out patrol leaves its bodies and vehicle for the players.
- **Escalation**: losses report a combat contact to Operations (`ReportCombatContact_S`), which decides any investigation, QRF or Crackdown change through its own rules.
- Spawning uses `AROR_ForceSpawner` (shared with Missions): faction preset lookups, group and vehicle spawning, seating, waypoints and road positions.
- Server/client: server only; clients see only the replicated AI. Saved: nothing; patrols are rebuilt around whoever is online.
- Depends on: Locations, Factions (presets and roles), Operations (Readiness, Crackdown, contact reports), Game Flow.
- Extension points: every count, distance, chance, timeout and prefab is an attribute on the component.
- Planned: supply convoys, checkpoints, hunter teams on reported contacts, helicopter patrols.

### Map Markers — In progress
Purpose: our own markers on any open map (player map, deploy screen, setup screen). Client only.

- `AROR_LocationMapIconsComponent` (game mode) draws an icon per location, tinted by the controlling faction's colour, and keeps it positioned as the map pans and zooms. Every location but cities shows its name to the right of the icon, vertically centred, bold near-black in the owner's hue (vanilla prints city names); the name is its own widget beside the icon so the icon frame cannot clip it. Icons are drawn as military-frame symbols (`AROR_MapFrameIcon`: rectangle ours, diamond enemy, glyph per type) unless the component switches back to textures. Strongholds, the campaign objectives, show the holding army's national flag instead (`AROR_ArmyFlag`), on the game map and in HICOMM. Each location prefab chooses its icon image (white, so it can be tinted).
- Hovering shows a pulsing selector; clicking opens an info card (name, type, controller; population and support for settlements) that updates live.
- QRF last-seen markers and incoming QRF/attack target markers (from Operations: a pulsing reticle ring on the threatened location with the army, response and arrival countdown; red for attacks, amber for QRFs) and active Resistance missions (from Missions) are currently drawn by the same component. Moving the map framework into its own system, with each feature providing its own layer, is under discussion.
- Not replicated or saved; reads replicated location state. No base-game marker configs are used.

### Player Map Markers — In progress
Purpose: player-authored notes, danger reports, rally points, supplies and numbered route points on the game map, managed through the FIELD phone app.

- `AROR_PlayerMarkersComponent` on the game mode owns annotations, author identity, sharing scope and optional expiry. Scopes are private, native player squad and Resistance-wide; only the author may edit or delete. Creation is bounded per author and per campaign.
- Server/client: requests travel through the owning player controller; the server validates identity, faction, campaign phase and bounds. Each client receives only annotations visible to them through owner-targeted snapshots. Native squad membership is checked again on every publication. `AROR_PlayerMapMarkers` draws the client map layer through Map Markers.
- Saved: private and Resistance-wide annotations, keyed by author identity, and remaining expiry time through `AROR_PlayerMarkersComponentSerializer`. Squad annotations are session-only because native groups are not persistent campaign identities.
- Depends on: Game Flow, Factions, High Command (map bounds), Map Markers, Phone, Persistence, Notifications.
- Extension points: marker limits on the component; extender maps use the same configurable map bounds as HICOMM.
- Native compilation and multiplayer acceptance remain pending; see `docs/FIELD_AND_CIVILIANS.md`.

### Personal Recruitment — In progress
Purpose: each Resistance player can raise and manage individual AI companions, separately from leader-owned High Command squads.

- `AROR_RecruitmentComponent` on the game mode purchases riflemen from the Resistance faction preset at HQ using faction funds and manpower. FIELD selects one recruit or the player's entire roster for follow, hold, regroup, vehicle passenger/driver/gunner assignment, dismount, nearby-player transfer and HQ dismissal.
- Server/client: the server owns the roster, spawn lifecycle, movement and seat requests. Player-controller RPCs validate ownership, faction, location, capacity and vehicle safety. Only the owner receives their roster summary. Native AI and vehicle replication supplies world entities.
- Saved: recruits themselves are session-only. Economy includes living and pending recruits' purchase value in saved funds/manpower, following HICOMM's refund policy. Failed formation, HQ dismissal, disconnect and campaign end return eligible value; casualties do not.
- Depends on: Game Flow, High Command (HQ registry), Factions, Economy, Phone, Notifications; spawning uses the shared Force Spawner.
- Extension points: recruitment limits, costs, HQ range, timeout and waypoint/group prefabs on the component; unit assets follow the selected Resistance preset.
- Native compilation, vehicle seating and multiplayer acceptance remain pending; see `docs/FIELD_AND_CIVILIANS.md`.

### Ambient Civilians — In progress
Purpose: populate towns and cities with walking civilians and occupied civilian cars while players are nearby.

- `AROR_CivilianManagerComponent` on the game mode activates settlement areas within a configurable radius, initially 1 km. Pedestrian destinations and road-traffic spawn/destination points remain inside that area. Cars carry a civilian driver and optionally a passenger, using the Civilians faction preset and the shared Force Spawner.
- Server/client: all population, spawning, seating, routes and retirement run on the server. Clients see native replicated AI and cars. Town populations are shared by all nearby players, with global people/vehicle budgets and paced construction. No dedicated-server UI is created.
- Civilians remain in the non-military Civilians role; they do not capture locations or generate military combat reports. Cleanup waits until players leave; claimed cars are retained. Spawn/seating timeouts cancel native queues and repeatedly stalled or failed routes retire without nearby despawning.
- Saved: nothing; ambient activity is rebuilt around players each session. Vehicles deposited into the Resistance Garage subsequently follow the garage's existing persistence rules.
- Depends on: Game Flow, Locations, Factions. No new faction assets or resource references are required.
- Extension points: enablement, activity/retention radii, per-town populations, global budgets, spawn distances, cleanup delay and group/waypoint prefabs.
- Native compilation, civilian driving/pathfinding and multiplayer acceptance remain pending; see `docs/FIELD_AND_CIVILIANS.md`.

### Economy — In progress
Purpose: money and manpower for the campaign: balances, and periodic income from held locations. General purchasing rules come later.

- `AROR_EconomyComponent` (game mode): **faction funds** per role, Resistance **manpower**, and **personal money** per player. Server-owned; funds and manpower replicate to everyone, a player's money only to that player. Player money is keyed by identity ID so it survives reconnects. Players can transfer personal money to another online player or donate it to Resistance funds (Bank Transfer RPCs on the player controller).
- `AROR_IncomeComponent` (game mode, server): every **15 minutes** (not during setup) each role is paid for what it holds:
  - Money = held Resources × $500 × (1 + 0.3 per held Factory + 0.5 per held Port), plus $5 per inhabitant of held towns and cities.
  - The Occupiers and Invaders also receive fixed **external support** ($500 each).
  - The Resistance gains manpower per held town and city, and every online Resistance player is paid 10% of the Resistance's money income as personal money (extra, not taken from the funds).
  - Resistance players get an income notification. Rates and the interval are attributes.
- Rules: balances never go below zero; spending succeeds only if the balance covers it. All changes happen on the server.
- Saved: funds, manpower and player money (`AROR_EconomyComponentSerializer`); Resistance funds and manpower include the refund for living High Command squads and living/pending personal recruits. Income timing is not saved.
- Used by: Operations (QRF costs), High Command (squad purchases), Missions (rewards), the phone's Bank app, admin tools.
- Extension points: server API to add, spend and read balances, and to trigger a payment.

### Resistance Storage — In progress
Purpose: the Resistance's shared item stock for the campaign (what gear is on hand), separate from Economy money and manpower.

- `AROR_ResistanceStorageComponent` (game mode): a ledger of **item prefab + quantity**. Any inventory item can be stocked; slot, type and volume stay on the item when it is later spawned or equipped. Starts empty.
- Server API: `Add_S` / `Take_S` (atomic) by prefab; `GetQuantity` / `GetEntries` on every machine; `AddEntity_S` deposits a world/inventory item by unpacking nested contents (vest cargo, weapon attachments, magazines, and so on) into **separate stock lines**, then deleting the entity. Magazines are credited by their **rounds**: leftover rounds pool per magazine type (server-only) and become stock as they add up to full magazines, so an emptied magazine is worth nothing. Item health is not preserved.
- Server-owned; stock replicates to all clients as parallel prefab/quantity arrays. Change invoker for future UI.
- Saved: the stock ledger and the pooled loose rounds (`AROR_ResistanceStorageComponentSerializer`).
- Admin: Economy → More can fill stock with one of every Resistance preset item (excl. units/vehicles).
- **Arsenal**: Prefab inherits vanilla `ArsenalBox_Base` plus `AROR_ResistanceArsenalComponent` (same entity as `SCR_ArsenalComponent`). Lists stocked prefabs (qty > 0), sorted by Resistance preset category then path, with stock quantity on each slot. Takes call `Take_S`; refunds call `AddEntity_S`. Supplies are never the budget (`IsArsenalUsingSupplies` forced off). Game mode has `SCR_ArsenalManagerComponent` so vanilla type flags do not disable the box; Resistance arsenals also force-enable. Modded `SCR_ArsenalComponent`, arsenal slot UI, and `SCR_ResourcePlayerControllerInventoryComponent` implement the bridge. Access: open the box like a normal arsenal. Empty stock shows an empty arsenal until admin Fill or deposits. Placement is not HQ-exclusive; Game Flow spawns the first one beside the HQ via `SpawnArsenal`.
- **Loadout slots**: `AROR_PlayerLoadoutsComponent` (game mode) holds **three** kits per player, keyed by identity ID (same as personal money) and saved with the playthrough (`AROR_PlayerLoadoutsComponentSerializer`). F-key actions (`AROR_SaveLoadoutSlotAction` / `AROR_LoadLoadoutSlotAction`) save a snapshot of worn gear or load a slot. Load **swaps with stock**: deposit current gear via `AddEntity_S`, take the kit’s flat prefab counts via `Take_S`, then re-equip with vanilla `SCR_PlayerArsenalLoadout` apply. Fails with a hint, before anything is deposited, if stock plus what the worn gear is worth (Storage's deposit rules) would be short. Slot occupancy is pushed only to the owning client. Magazine and item health handling follow Storage. Wire the six actions in Workbench onto the arsenal’s **inherited** `ActionsManagerComponent` as `additionalActions +` (do not add a second ActionsManager — that breaks the prefab / spawn).

### Resistance Garage — In progress
Purpose: the Resistance's shared vehicle garage for the campaign — unique stored vehicles with condition preserved, separate from Resistance Storage (fungible item stock).

- `AROR_ResistanceGarageComponent` (game mode): a ledger of **unique vehicle entries** (monotonic id + prefab + per-hit-zone health + fuel by tank id + cargo tree + mounted weapon ammo). Any world vehicle may be stored (not limited to Resistance presets); Occupiers/Invaders have no garage.
- **Capacity**: 10 slots from the Resistance HQ (`m_iHqSlots`) plus 2 slots per Resistance-controlled military objective (`m_iObjectiveSlots`, `IsCapturableType` only — towns and cities excluded). Recomputed from the location registry. If territory is lost and count exceeds capacity, existing entries are kept and new stores are blocked until under capacity.
- Server API: `StoreVehicle_S` (snapshot + despawn; rejects destroyed/occupied/full), `RequestStoreVehicle_S` (player store with site/range checks), `DiscardVehicle_S`, `RetrieveVehicle_S` (spawn within place radius of a garage anchor, restore snapshot, affiliate **Resistance**, remove entry), soft `ReserveEntry_S` / `ReleaseReservation_S` while placing. `GetCapacity` / `GetCount` / `GetEntries` on every machine.
- **Store (looked-at)**: phone home quick dock slot 1 calls `AROR_GarageStoreAPI.TryStoreLookedAtVehicle()` (then closes the phone). Raycasts from the local character camera (max **5 m**, `m_fStoreLookDistance`), only when the player is within **50 m** (`m_fStoreSiteRadius`) of a Resistance **HQ** or capturable military site (towns/cities excluded). RPC `AROR_RequestGarageStore`.
- **Retrieve UI**: `AROR_GarageRetrieveUI.Open(anchor)` opens a simple configurable dialog (browse Prev/Next, Spawn). Spawn starts a client ghost placement mode (`AROR_GaragePlacementController`) with an overhead camera — place within **50 m** (`m_fPlaceRadius`) of the garage anchor on clear ground (Space place, Esc/RMB cancel, Q/E rotate; the ghost is excluded from the clearance TraceBox). `AROR_GarageRetrieveAction` on `AROR_Prop_Garage` (retrieve only). Player-controller RPCs carry detail/reserve/retrieve.
- **HQ garage prop**: `AROR_Prop_Garage` inherits `PalletFuel_01` with Persistence; retrieve action on its Workbench `ActionsManagerComponent` (never add a second ActionsManager or Rpl by hand). Game Flow spawns it opposite the arsenal at HQ creation (`m_vArsenalOffsetFromHQ`, default 0.5 m either side of the flag).
- Server-owned full snapshots; compact id/prefab/health/fuel summaries replicate. Change invoker for UI.
- Saved: full entries (`AROR_ResistanceGarageComponentSerializer`). Reservations are session-only.
- Depends on: Game Mode, Locations, Location Capture (capturable type filter), Persistence, Notifications (hints).

### Phone — In progress
Purpose: an in-game phone that hosts player-facing apps. Client only.

- `AROR_PhoneMenu` (a menu registered in `chimeraMenus.conf`) and `AROR_PhoneComponent` (game mode), which opens it with a rebindable keybind (**Ctrl+Y**) for living players when no other menu is open. Esc closes it.
- A phone frame over a wallpaper with a status bar. Apps open in portrait, or turn the whole phone to landscape and enlarge it. The frame's home button returns to the home screen.
- **Quick dock**: four slots along the bottom of the home screen. Slot 1 stores the looked-at vehicle (Resistance Garage) and closes the phone; slots 2–4 are empty placeholders for now.
- Apps: **Bank** (account holder, personal money, Resistance funds, live from the Economy; Transfer screen sends personal money to an online player or Resistance funds), **REBINTEL** (one tab per enemy army: Readiness and Crackdown cards in that army's labels with a 5-segment meter and what the level means; the player's HUD widget and alert settings; read-only: the levels are driven by the campaign), **HICOMM** (landscape High Command app for Resistance Leaders: leadership, a zoomable map for orders, squad purchases; the map shows the base game map by default with military-frame location symbols, the procedural tactical map behind a toggle), **OPS** (landscape mission board from Missions: browse operations, dossier with an area map; leaders accept and abort), **Black Market** (landscape smugglers' shop from Black Market: access ladder, vehicle and firearm tabs, two-step ordering, order list), **FIELD** (landscape personal map annotations and AI companion management) and **MARS** (landscape response debugger from Operations, replacing the temporary Test tile).
- Depends on: Economy, Notifications (hints), Operations (RebIntel), High Command (HICOMM), Missions (OPS), Resistance Garage (quick store).
- Saved: REBINTEL player settings (HUD widget on/off, compact/full, top left/right, alert pop-ups, tracked enemy) in the player's own profile folder, client-side (`AROR_RebIntelSettings`). Nothing on the server.
- Planned: app registration so extender mods can add apps; a Settings app and Feed (see `docs/UI_DESIGN.md`).

### Notifications — In progress
Purpose: tell players what is happening in the campaign, plus quiet personal feedback. Any system can send them.

- **Headline** (alert banner, top-centre below the compass: priority-coloured header with its source, headline, optional detail line), **Notification** (toast on the right edge, routine events) and **Hint** (bottom pill, personal feedback such as why an action failed).
- `AROR_NotificationManagerComponent` (game mode) with a static API; `AROR_NotificationDisplay` draws them.
- **REBINTEL status widget** (`AROR_RebIntelHud`, client): both enemy armies' Readiness and Crackdown in a top corner, each army with its national flag, coloured by stage (green to red), compact or full, hidden while a menu or the map is open. Level changes raise local 5-second banners (`AROR_RebIntelAlerts`): orange for Crackdown up, amber for Readiness up, green for drops, if the player allows pop-ups.
- Server/client: headlines and notifications are sent by the server to everyone or one player. Hints, the status widget and level-change alerts are local. Drawing is client-only.
- Headlines and notifications share one queue (headlines first) and show to all players, alive or dead. Hints replace each other and are dropped while the player is dead. Nothing is saved or replayed.
- Planned: priorities P1–P4 as their own field (sound, duration, opt-out), duplicate merging, a notification log (HICOMM Feed).

### Admin Tools — In progress
Purpose: in-game testing and admin tools, opened with **Ctrl+T** (`AROR_AdminMenuComponent`, game mode).

- Sub-menus: teleport, Game Master, settlement support, test notifications, and economy (give money, pay income now, fill Resistance stock from the Resistance preset).
- Each tool asks the server, which applies the change.
- Packaged sessions require an administrator or session-administrator role for the local menu and every server mutation. `AROR_AdminAccess` supplies the same role policy for MARS; a UI credential cannot authorize gameplay changes. Workbench retains local testing access. Native client-role replication and permission-revocation acceptance remain pending.

### Map Templates — In progress
Purpose: per-map setup of the game mode, as a sub-scene of the terrain's world.

- One sub-scene per map with its own mission header. First map: **Everon** (`Worlds/AROR_Everon/`, mission `Missions/AROR_Everon.conf`).
- Each map places the game mode, faction and loadout managers, a staging spawn and its location prefabs.
- Developer test sub-scenes (`AROR_Everon_Rowantrek`, `AROR_Everon_Chicken`) carry their own test settings. Chicken holds an island-wide catalogue of 41 campaign locations generated from `docs/EVERON_CAMPAIGN_LOCATIONS.csv` (see `docs/EVERON_CAMPAIGN_LOCATIONS.md`).
- Extender map mods create a sub-scene of their own terrain and place location prefabs; no code needed.

<!--
### System name — Status
Purpose: one or two sentences.
Key classes/prefabs/configs: `AROR_...`, `Prefabs/...`, `Configs/...`
Server/client: what runs where and what is replicated.
Saved: what is persisted.
Depends on: other systems.
Extension points: what extender mods can override or add.
Planned: what is designed but not built.
-->
