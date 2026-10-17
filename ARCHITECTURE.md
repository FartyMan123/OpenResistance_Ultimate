# Architecture

Tracks the architecture of Open Resistance: its systems, sub-systems, and how they fit together. Update this file whenever a system is added, removed, or changes shape.

Status tags: **Planned** (designed, not built) · **In progress** · **Implemented**.

## Mod structure

### AR-Open-Resistance (CORE MOD)

All the scripts and functionality, plus templates for base-game assets.

- Lives in this repo: `AROpenResistance/Open Resistance/` (addon ID `OpenResistance`, GUID `6AA282336705EED2`).
- Class prefix: `AROR_` for all core classes, enums, and globals (e.g. `AROR_CacheComponent`). Reserved for the core mod.
- Depends only on the base game (`ArmaReforger`, `58D0FB3206B6F859`). It must stay playable on its own.
- Provides:
  - every game mode system (see [Systems](#systems))
  - template prefabs and configs built from base-game assets (caches, intel, spawn points, factions, loadouts, AI groups) that extender mods can inherit from or replace
  - the extension points that extender mods use

### Extender Mods (made by others)

Separate addons, maintained by others, that add extra content such as maps, weapons, vehicles, scripts, functions, missions etc.

- Each one lists the core mod (`6AA282336705EED2`) as a dependency in its `addon.gproj`.
- Extends the core by:
  - inheriting from core template prefabs and configs
  - supplying their own configs (factions, loadouts, AO/cache data for new maps, missions)
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
- `Prefabs/GameModes/AROR_GameMode_Base.et`: inherits base-game `{1B76F75A3175E85C}Prefabs/MP/Modes/Plain/GameMode_Plain.et` (`SCR_BaseGameMode`) and adds `AROR_LocationManagerComponent`. New system components are added here.
- No custom script class yet; `AROR_InsurgencyGameMode : SCR_BaseGameMode` will be added when game mode logic is needed.

Server/client: standard `SCR_BaseGameMode` replication.

Extension points: extender mods can inherit `AROR_GameMode_Base.et` to add or swap components.

Sub-systems (Planned):
- `AROR_GameMode_Insurgency.et` inheriting the base, with the Insurgency rules (caches, AO, tickets).
- A playable world also needs faction manager, loadout manager and spawn point entities; these will be part of the map template.

### Locations — In progress
Purpose: named areas on the map (starting with **Town**) that other systems use to decide where things happen: AO selection, cache spawning, AI spawning.

Key classes/prefabs/configs:
- `Scripts/Game/Locations/AROR_ELocationType.c`: location type enum (`TOWN`).
- `Scripts/Game/Locations/AROR_LocationComponent.c`: on each placed location. Name, type, and **per-town radius** (editable per instance in the World Editor; the radius is drawn as an orange ring in Workbench). The centre is the entity's position.
- `Scripts/Game/Locations/AROR_LocationManagerComponent.c`: on the game mode entity. Registry and queries: all locations, by type, nearest, location at a position.
- `Prefabs/Locations/AROR_Location_Base.et` → `AROR_Location_Town.et`: placeable templates. The Town prefab uses the component defaults (`TOWN`, 250 m).

Server/client: locations are static world entities and register on both server and clients. Nothing is replicated yet; dynamic state (control status, active/inactive) will be server-owned and replicated when added.

Depends on: Map Templates (for placement).

Extension points:
- New location types via `modded enum AROR_ELocationType` plus a prefab inheriting `AROR_Location_Base.et`.
- Extender maps place `AROR_Location_*` prefabs in their own sub-scene.

Sub-systems (Planned):
- Town state (insurgent-held / contested / cleared).
- Optional hand-placed child points (cache, AI spawn, garrison); fall back to scanning buildings inside the radius.
- Default name from the nearest base-game map descriptor.
- Workbench plugin to auto-place towns on a terrain.

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
