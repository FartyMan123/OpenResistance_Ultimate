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

### Base-game config overrides

The core adds entries to two base-game configs with **Override in addon**, which stores only our additions on top of the base file: `Configs/System/chimeraMenus.conf` (the phone menu) and `Configs/System/chimeraInputCommon.conf` (the phone keybind). Everything else uses our own configs.

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
- Everon campaign data: a catalogue of 41 stable locations (18 settlements, 23 military installations) generated into the Chicken world layers by scripts in `tools/`. See `docs/EVERON_CAMPAIGN_LOCATIONS.md`.

### Location Capture — In progress
Purpose: how a location's controlling role changes. All changes are made by the server through the location's control; the HQ cannot be captured.

- **Towns & Cities**: held by the role with **more than 50% support** (see Settlements).
- **Military locations** (airport, factory, military base, port, resources, outpost): captured at the location's **capture flag** by interaction, only when **every hostile** inside the radius is dead.
- **Settlements are support-only**: towns and cities reject flag capture and AI recapture; they change hands only through support (see Settlements).
- **Flip cooldown**: after a Resistance capture, the location cannot flip back to the Occupiers or Invaders for **5 minutes**. Replicated, not saved.
- **AI recapture** (`AROR_RecaptureComponent`, server): every few seconds the forces inside each capturable location are weighed; defenders count ×2 and attacking AI ×1. An attacking AI role that outweighs the defenders takes the location, subject to the cooldown.
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
- Defence layouts (`AROR_DefenceLayout`, `AROR_DefencePositionComponent` and the `AROR_Defence_*` compositions) define where defenders and vehicles stand. Editor tools audit and ground these sites.
- Server/client: roster, spawning and saves are server-owned; clients see the replicated defenders. A replicated gate stops capture while a defence is still deploying.
- Saved: the roster (what remains and what was lost) per location, through Persistence. Defenders are not refilled when players leave and return.
- Depends on: Locations, Location Capture, Factions, Operations (damage reporting).
- Extension points: scenario and editor settings (activation range, capacity, defence levels, layouts); role asset catalogues can be replaced for extender factions.
- Planned: preset-driven units and vehicles (see Factions), restoring exact inventory and live positions, replenishment.
- Feature detail: `docs/GARRISONS_AND_REGIONAL_SUPPORT.md`, `docs/MILITARY_DEFENCE_PRESETS.md`.

### Game Flow — In progress
Purpose: the lifecycle from world boot to play, separating **scenario data** (from the world, never saved) from **playthrough data** (from a save).

Phases (mapped onto the base-game states PREGAME → GAME → POSTGAME):
1. **Scenario load**: world boots, locations register, location markers appear.
2. **Setup** (PREGAME): the server admin creates a new playthrough (choosing a faction preset per role, or a quick start with the base-game presets, then picking the Resistance HQ position on the map) or loads a saved one. A server config fallback for unattended dedicated servers is planned.
3. **Playthrough load**: new → fresh state; load → restore from the save. Then GAME.
4. **Running**: state changes are saved (see Persistence).

- `AROR_GameFlowComponent` (server, game mode) holds PREGAME until a playthrough exists and keeps player controls disabled until then; other systems can ask whether the playthrough is ready.
- **Setup UI** (`AROR_SetupUIComponent`, client) is built from base-game configurable dialogs with our own presets (`Configs/UI/AROR_SetupDialogs.conf`).
- **Staging spawn** (`AROR_StagingSpawn.et`): before a playthrough exists, players spawn there as the Resistance and move freely. Afterwards it is disabled and players are moved to the HQ.
- **Resistance HQ**: placed on land, outside every location's radius, and on a clear 10×10 m pad free of buildings and props (checked on the client, enforced by the server; invalid picks are rejected). A dynamic location of type HQ that carries the Resistance spawn point; players always spawn there. A new playthrough also spawns an arsenal beside the HQ via `SpawnArsenal` before the initial save.
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
- Saved so far: location control, settlement support, the Resistance HQ, arsenals, faction presets per role, faction funds, manpower, player money, Resistance item stock, per-player arsenal loadout slots, garrison rosters, QRF war level, aggression and debt, and the Invader campaign state.
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
- A preset (`AROR_FactionPreset`, one `.conf` each) lists what an army makes available in eight fixed categories: **Units** (AI characters, at most one per unit type), **Vehicles** (empty, no crew), **Gear**, **Equipment**, **Weapons**, **Weapon accessories**, **Ammunition** and **Vehicle ammunition** (munitions loaded onto vehicles and aircraft as items, such as helicopter rockets; vehicle-mounted guns rearm through the supply system instead). Every entry is a prefab with a **base cost** and a **minimum war level** (default 1) when it becomes available, grouped by type (e.g. RIFLEMAN, MOTOR_TRUCK, RIFLE); apart from units, a type can have any number of entries.
- Presets only make things available; the systems that use them (garrisons, QRFs, shops, recruitment) decide what to pick. Each preset lists the roles it can be used for.
- `AROR_FactionPresetRegistry` (`Configs/FactionPresets/AROR_FactionPresetRegistry.conf`) lists the available presets. The base-game presets (`FIA`, `US_ARMY`, `USSR_ARMY`, `EVERON_CIV`) are built into the core, written the same way an extender would.
- `AROR_FactionPresetsComponent` (game mode) loads the presets on every machine and answers queries for a role's preset. The server owns which preset each role uses: chosen by the admin per role when creating a campaign (defaults otherwise), replicated, and saved (`AROR_FactionPresetsComponentSerializer`). A saved preset that no longer exists falls back to the role's default.
- Planned: garrisons, QRFs and air QRFs read their units and vehicles from presets instead of their own lists.
- To verify: whether Override in addon merges additions into the registry's plain path list or replaces it (named entries are proven to merge, plain arrays are not). Needs a test with two addons; if it replaces, the registry switches to named entries.

- `AROR_LoadoutManager.et` offers only Resistance loadouts; `AROR_Character_Resistance_Base.et` is the player character.
- Extension points: extender mods add presets (new `.conf` files listed in the registry through Override in addon). The categories and their types are a fixed core list; entries with any other type are ignored. Server admins or extenders point a role at another faction key without touching core code.

### Commanders — In progress
Purpose: one server-owned command context per role. The Resistance is player-directed; the Occupiers and Invaders are AI-directed through the same evaluation interface.

- `AROR_Commander` (role, faction, control mode) and `AROR_CommanderManagerComponent` (game mode), which periodically offers each AI commander for evaluation once the playthrough is ready.
- `AROR_InvaderCampaignComponent` (optional, game mode): simulates resource-funded Occupier/Invader engagements over distant military sites, expanding the Invaders from an offshore support corridor. It spends Economy funds and changes control through the normal location authority; it never fights near players. Chicken opts in.
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

### Operations (Attacks & QRF) — In progress
Purpose: enemy **attacks** and defensive **quick reaction forces (QRF)** for the Occupiers and Invaders: deciding when to respond, choosing and paying for a force, spawning and directing it, and cleaning up afterwards.

Main parts:
- **Scheduling** (`AROR_OperationManagerComponent`): accepts attack requests and threat reports, ranks threats, applies per-role cooldowns, capacity and deduplication, and tracks each operation from planned to a final outcome. Invader operations are **disabled by default** until territory and resource integration is ready.
- **Force selection**: designer-authored force profiles and ground recipes per role and faction (motorized, mechanized, armour, airborne), response levels 1–5, and an optional strategy layer: saved **war level** decides which equipment a response may use, and a separate saved **aggression** sets its strength (1-5).
- **Funding** (`AROR_QRFFinance`): responses reserve and spend the role's funds through the **Economy** API; losses are charged and shortfalls become saved QRF debt. Economy remains the only bank.
- **Execution** (`AROR_OperationExecutorComponent` and helpers): spawns and reserves forces, delivers them by road or by air, runs searches and town security, and staged reinforcement waves under a shared tactical commander (`AROR_QRFTacticalCommander`). Forces launch from friendly outposts, bases or airports.
- **Airborne execution** (experimental, `AROR_ReaperQRFExecutor`): helicopter insertion and gunship support using the REAPER mod; can be switched off.
- **Defence and spotting**: assigned defenders report hostile damage; capture events can trigger a response for the previous owner. Resistance players who see QRF vehicles get last-seen map markers.

- Server/client: all decisions and execution run on the server. Clients only receive QRF spotting reports (their own, through the player controller), send whether their map is open, and read a public war level/aggression summary (RebIntel).
- Saved: war level, aggression and QRF debt only. Live operations, deployed forces, queues and cooldowns are session-only.
- Depends on: Commanders, Locations, Location Capture, Factions, Economy, Game Flow; the REAPER mod for airborne execution.
- Extension points: scenario and prefab configuration of schedules, limits, force profiles, recipes and prices; trusted server callers can request attacks and report threats.
- Feature detail: `docs/OPERATIONS.md`, `docs/QRF_STRATEGY_ECONOMY.md`, `docs/QRF_TACTICAL_COMMANDER.md`, `docs/AIRBORNE_QRF_PROTOTYPE.md`, `docs/QRF_SPOTTING.md` and the other QRF files in `docs/`. Testing in the Chicken world: `docs/OPERATION_EXECUTOR.md`, `docs/FORCE_POOL_QUICKSTART.md`, `docs/TRUCK_IFV_QRF_TEST.md`.
- Planned: autonomous target selection, physical military stock, durable save/restore of live operations, client summaries.

### Map Markers — In progress
Purpose: our own markers on any open map (player map, deploy screen, setup screen). Client only.

- `AROR_LocationMapIconsComponent` (game mode) draws an icon per location, tinted by the controlling faction's colour, and keeps it positioned as the map pans and zooms. Each location prefab chooses its icon image (white, so it can be tinted).
- Hovering shows a pulsing selector; clicking opens an info card (name, type, controller; population and support for settlements) that updates live.
- QRF last-seen markers (from Operations) are currently drawn by the same component. Moving the map framework into its own system, with each feature providing its own layer, is under discussion.
- Not replicated or saved; reads replicated location state. No base-game marker configs are used.

### Economy — In progress
Purpose: money and manpower for the campaign: balances, and periodic income from held locations. General purchasing rules come later.

- `AROR_EconomyComponent` (game mode): **faction funds** per role, Resistance **manpower**, and **personal money** per player. Server-owned; funds and manpower replicate to everyone, a player's money only to that player. Player money is keyed by identity ID so it survives reconnects. Players can transfer personal money to another online player or donate it to Resistance funds (Bank Transfer RPCs on the player controller).
- `AROR_IncomeComponent` (game mode, server): every **15 minutes** (not during setup) each role is paid for what it holds:
  - Money = held Resources × $500 × (1 + 0.3 per held Factory + 0.5 per held Port), plus $5 per inhabitant of held towns and cities.
  - The Occupiers and Invaders also receive fixed **external support** ($500 each).
  - The Resistance gains manpower per held town and city, and every online Resistance player is paid 10% of the Resistance's money income as personal money (extra, not taken from the funds).
  - Resistance players get an income notification. Rates and the interval are attributes.
- Rules: balances never go below zero; spending succeeds only if the balance covers it. All changes happen on the server.
- Saved: funds, manpower and player money (`AROR_EconomyComponentSerializer`); Resistance funds and manpower include the refund for living High Command squads. Income timing is not saved.
- Used by: Operations (QRF costs), High Command (squad purchases), the phone's Bank app, admin tools.
- Extension points: server API to add, spend and read balances, and to trigger a payment.

### Resistance Storage — In progress
Purpose: the Resistance's shared item stock for the campaign (what gear is on hand), separate from Economy money and manpower.

- `AROR_ResistanceStorageComponent` (game mode): a ledger of **item prefab + quantity**. Any inventory item can be stocked; slot, type and volume stay on the item when it is later spawned or equipped. Starts empty.
- Server API: `Add_S` / `Take_S` (atomic) by prefab; `GetQuantity` / `GetEntries` on every machine; `AddEntity_S` deposits a world/inventory item by unpacking nested contents (vest cargo, weapon attachments, magazines, and so on) into **separate stock lines**, then deleting the entity. Magazine fill and item health are not preserved.
- Server-owned; stock replicates to all clients as parallel prefab/quantity arrays. Change invoker for future UI.
- Saved: the stock ledger (`AROR_ResistanceStorageComponentSerializer`).
- Admin: Economy → More can fill stock with one of every Resistance preset item (excl. units/vehicles).
- **Arsenal**: Prefab inherits vanilla `ArsenalBox_Base` plus `AROR_ResistanceArsenalComponent` (same entity as `SCR_ArsenalComponent`). Lists stocked prefabs (qty > 0), sorted by Resistance preset category then path, with stock quantity on each slot. Takes call `Take_S`; refunds call `AddEntity_S`. Supplies are never the budget (`IsArsenalUsingSupplies` forced off). Game mode has `SCR_ArsenalManagerComponent` so vanilla type flags do not disable the box; Resistance arsenals also force-enable. Modded `SCR_ArsenalComponent`, arsenal slot UI, and `SCR_ResourcePlayerControllerInventoryComponent` implement the bridge. Access: open the box like a normal arsenal. Empty stock shows an empty arsenal until admin Fill or deposits. Placement is not HQ-exclusive; Game Flow spawns the first one beside the HQ via `SpawnArsenal`.
- **Loadout slots**: `AROR_PlayerLoadoutsComponent` (game mode) holds **three** kits per player, keyed by identity ID (same as personal money) and saved with the playthrough (`AROR_PlayerLoadoutsComponentSerializer`). F-key actions (`AROR_SaveLoadoutSlotAction` / `AROR_LoadLoadoutSlotAction`) save a snapshot of worn gear or load a slot. Load **swaps with stock**: deposit current gear via `AddEntity_S`, take the kit’s flat prefab counts via `Take_S`, then re-equip with vanilla `SCR_PlayerArsenalLoadout` apply. Fails with a hint if stock would be short. Slot occupancy is pushed only to the owning client. Magazine fill and item health follow Storage (not preserved). Wire the six actions in Workbench onto the arsenal’s **inherited** `ActionsManagerComponent` as `additionalActions +` (do not add a second ActionsManager — that breaks the prefab / spawn).

### Phone — In progress
Purpose: an in-game phone that hosts player-facing apps. Client only.

- `AROR_PhoneMenu` (a menu registered in `chimeraMenus.conf`) and `AROR_PhoneComponent` (game mode), which opens it with a rebindable keybind (**Ctrl+Y**) for living players when no other menu is open. Esc closes it.
- A phone frame over a wallpaper with a status bar. Apps open in portrait, or turn the whole phone to landscape and enlarge it. The frame's home button returns to the home screen.
- Apps: **Bank** (account holder, personal money, Resistance funds, live from the Economy; Transfer screen sends personal money to an online player or Resistance funds), **RebIntel** (read-only enemy war level and aggression from Operations), **HICOMM** (landscape High Command app for Resistance Leaders: leadership, a zoomable map for orders, squad purchases) and a landscape test app.
- Depends on: Economy, Notifications (hints), Operations (RebIntel), High Command (HICOMM).
- Planned: app registration so extender mods can add apps.

### Notifications — In progress
Purpose: tell players what is happening in the campaign, plus quiet personal feedback. Any system can send them.

- **Headline** (full-width strip, major events), **Notification** (dispatch card, routine events) and **Hint** (bottom pill, personal feedback such as why an action failed).
- `AROR_NotificationManagerComponent` (game mode) with a static API; `AROR_NotificationDisplay` draws them.
- Server/client: headlines and notifications are sent by the server to everyone or one player. Hints are shown locally, or sent by the server to one player. Drawing is client-only.
- Headlines and notifications share one queue (headlines first) and show to all players, alive or dead. Hints replace each other and are dropped while the player is dead. Nothing is saved or replayed.
- Planned: more audiences (roles, areas) and a notification log.

### Admin Tools — In progress
Purpose: in-game testing and admin tools, opened with **Ctrl+T** (`AROR_AdminMenuComponent`, game mode).

- Sub-menus: teleport, Game Master, settlement support, test notifications, and economy (give money, pay income now, fill Resistance stock from the Resistance preset).
- Each tool asks the server, which applies the change.
- Currently available to **every player** while "Menu Available" is on; **restricting it to server admins is required before public servers**, both for opening the menu and for the server accepting its requests.

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
