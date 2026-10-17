# CLAUDE.md

Guidance for Claude Code when working in this repository.

> **Do not modify this file (CLAUDE.md) without explicit permission from the user.** If something here looks outdated or wrong, propose the change and wait for approval before editing.

## Project

**Open Resistance** is an Arma Reforger (Enfusion engine) mod: an Insurgency-style game mode. Players on the occupying/counter-insurgency side hunt hidden weapon caches across a map while an insurgent force (AI, and optionally players) defends them, ambushes patrols, and contests territory. The round is won by destroying the required number of caches, or lost when the counter-insurgency side runs out of tickets/time.

The project is at an early stage — only the addon project file exists. Treat the gameplay design below as the intended direction, not as implemented behaviour, and check the code before assuming a system exists.

### Core gameplay loop (target design)

- **Caches**: spawned at randomised, valid locations (buildings/compositions) inside the active area of operations. Destroying one (explosives) advances the game.
- **Area of Operations (AO)**: only a subset of the map is active at a time; once its caches are cleared, the AO moves on.
- **Intel**: killed insurgents can drop intel; collecting it reveals approximate cache locations (map markers with an uncertainty radius that shrinks with more intel).
- **Insurgent AI**: spawns around the AO and the caches, out of player sight, with patrols, garrisons, and reinforcements scaled to player count.
- **Respawn and tickets**: counter-insurgency players respawn at a FOB or base and use a shared ticket pool; insurgent players (if enabled) respawn near caches or safehouses.
- **Win/lose**: all caches destroyed → counter-insurgency wins; tickets or time exhausted → insurgents win.

## Architecture

[ARCHITECTURE.md](ARCHITECTURE.md) is used to track the architecture of the mod: its systems and sub-systems, and how the core mod relates to extender mods. Read it before working on a system. Keep it up to date when systems are added, removed, or changed. Unlike this file, you may update it without asking first.

## Repository layout

```
AROpenResistance/
  Open Resistance/          # The Workbench addon root (open addon.gproj in Workbench)
    addon.gproj             # Addon definition: ID "OpenResistance", depends on ArmaReforger (58D0FB3206B6F859)
    resourceDatabase.rdb    # Workbench-generated resource DB — gitignored, never hand-edit
```

Repo-level `.gitignore` excludes Workbench caches/backups; `.gitattributes` keeps Workbench text resources CRLF on checkout and marks art/audio as binary (Git LFS lines are prepared but commented out until custom assets are added).

When adding content, follow the standard Reforger addon structure inside `AROpenResistance/Open Resistance/`:

| Folder | Contents |
|---|---|
| `Scripts/Game/` | Enforce Script (`.c`) game logic — the bulk of the mod |
| `Prefabs/` | Entity prefabs (`.et`): cache, intel item, spawn points, game mode entity |
| `Configs/` | Config resources (`.conf`): factions, loadouts, AI group lists, game mode settings |
| `Worlds/` | Scenario/sub-scene worlds (`.ent`, `.layer`) |
| `Missions/` | Mission headers (`.conf`) that make scenarios appear in the server/scenario list |
| `UI/` | Layouts (`.layout`) and imagesets |
| `Language/` | Localisation string tables |

## Working with Enfusion files

- **No command-line build.** Scripts are compiled and the game is run from Arma Reforger Tools (Workbench). Claude cannot compile or test here — say so when a change is unverified and tell the user what to check in Workbench (Script Editor compile, then Play in World Editor).
- **`.meta` files and GUIDs**: every resource has a sibling `.meta` file containing its GUID. Resources are referenced as `"{GUID}path/to/file.ext"`. Never invent GUIDs for references to existing assets; when creating new resources, prefer having the user create them in Workbench so the `.meta` is generated, or clearly flag hand-written GUIDs.
- Do not edit `resourceDatabase.rdb` or anything Workbench regenerates.
- Prefabs, configs, and worlds are text formats and can be edited, but keep diffs minimal — Workbench rewrites them on save, so a round-trip through Workbench may reformat them.
- Base game sources (Script Editor / "Open Resource" in Workbench) are the reference for any `SCR_` class. When unsure of an API, ask the user to check it or quote the base-game source rather than guessing signatures.

## Enforce Script conventions

- **Class prefix**: use `AROR_` for all new classes, enums, and globals (e.g. `AROR_CacheComponent`, `AROR_InsurgencyGameMode`). Filenames match class names (`AROR_CacheComponent.c`). The prefix is reserved for the core mod; extender mods should use their own prefix.
- **Extending the base game**: subclass where possible (e.g. `AROR_InsurgencyGameMode : SCR_BaseGameMode`). Use `modded class` only when changing base behaviour that cannot be subclassed, and keep modded overrides small and calling `super`.
- **Components**: pair every component with its class definition:
  ```c
  [ComponentEditorProps(category: "OpenResistance", description: "...")]
  class AROR_CacheComponentClass : ScriptComponentClass {}

  class AROR_CacheComponent : ScriptComponent
  {
      [Attribute("1", desc: "...")]
      protected int m_iSomething;
  }
  ```
- **Naming** (match Bohemia style): members `m_` with a type hint (`m_iCount`, `m_fRadius`, `m_bActive`, `m_sName`, `m_aCaches`, `m_mLookup`), statics `s_`, constants `UPPER_CASE`, methods `PascalCase`. Tabs for indentation, braces on their own line.
- Use `ref` for owned object members/arrays of objects; avoid strong ref cycles.
- Expose designer-tunable values via `[Attribute]` instead of hard-coding them, so they can be set on prefabs/configs.

## Multiplayer and replication

This is a multiplayer game mode; assume a dedicated server with many clients.

- **The server is authoritative** for all game state: cache spawning and destruction, AO selection, intel rolls, tickets, AI spawning, win conditions. Guard such logic with `Replication.IsServer()` or an `RplComponent` role check.
- Replicate state to clients with `[RplProp(onRplName: "...")]` and `Replication.BumpMe()`; use `[RplRpc(...)]` for events. Clients request actions through RPCs to the server — never trust client-side state.
- Entities that replicate need an `RplComponent` on their prefab.
- Keep per-frame work (`EOnFrame`, `OnUpdate`) out of hot paths; prefer `GetGame().GetCallqueue().CallLater(...)` timers for periodic checks (AO updates, AI scaling).
- Client-only code (UI, markers, HUD) must not run on a dedicated server — check `RplSession.Mode() != RplMode.Dedicated` where relevant.

## Git

- Development happens on `master-dev`; PRs target `master`.
- Every commit and PR must be well formatted and carry relevant information:
  - **Commit messages**: a concise imperative subject line (~50 chars, e.g. `Add cache spawning to AO system`), a blank line, then a body explaining *what* changed and *why*, naming the affected systems (as in [ARCHITECTURE.md](ARCHITECTURE.md)). One logical change per commit.
  - **Pull requests**: a clear title and a description with sections for **Summary** (what and why), **Changes** (bullet list by system or file area), **Testing** (what was checked in Workbench, or that it is untested), and **Notes** (replication or performance impact, breaking changes for extender mods, follow-ups). Link related issues.
  - Call out anything that affects extender mods (changed templates, renamed classes, altered configs) explicitly in both.
