# Open Resistance - WCS (test project)

An empty project whose only job is to load the Worst Case Scenario (WCS) content addons together with the core in Workbench, so the WCS integration can be tested. It holds no scripts or configs. On a server you just add WCS to the mod list; this project is not needed.

## How WCS gets into the game mode

WCS adds its weapons, vehicles and helicopters to the base-game US, USSR and FIA faction catalogs. The core faction presets augment themselves from those catalogs at runtime (ARCHITECTURE.md → Factions → catalog augmentation):

- **US Army** and **Soviet Army** gain every catalog asset they do not list, with a default Readiness by type: armed cars, transport helicopters and static AT at 2; APCs, IFVs and static AA at 3; armed helicopters and aircraft munitions at 4; tanks at 5.
- **FIA** (Resistance) gains the hand-held weapons, accessories and ammunition of the US and USSR catalogs that exist neither in the base game, RHS nor WCS RHS Weapons, i.e. the WCS-made ones. Guided launchers (Javelin, Stinger, Igla) are left out by name. Base-game, RHS and WCS RHS weapons and the guided launchers are left for the Black Market.

The systems then use the augmented presets:

| System | What WCS adds |
|---|---|
| QRFs (Chicken profiles) | Trucks, APCs, IFVs (armour profiles), armed cars, transport helicopters and gunship escorts picked per dispatch at the army's Readiness; crews sized to the hull; authored vehicle as fallback |
| Patrols | Cars, armed cars and APCs by Readiness (unchanged code: already preset-driven) |
| Missions | Convoy/supply trucks and escorts, limited to the army's Readiness |
| Garrisons | Static MGs, armed cars and APCs appended to the manned/static choices |
| Black Market | Every army vehicle (helicopters and tanks included) and the army firearms the Resistance does not already have, by port tier |
| Arsenal (admin fill) | The WCS-made firearms in the FIA preset |

## Dependencies

The core (`6AA282336705EED2`), RHS: Status Quo, and the WCS content addons: Core, Arsenal, Armaments, Weapons, Attachments, Scopes, Clothing, NATO, RU, RHS Weapons/Attachments/Scopes/Vehicles, Vehicles, Stryker, FMTV, JLTV (+ Upgrade), MRZR, Motorbikes, ZU-23-2, KBA-105, Helicopters, AH-1S, AH-64D (+ Upgrade), AH-6M, Mi-24V (+ Upgrade) and KA-52 (+ Upgrade). WCS server-gameplay addons and maps are deliberately left out.

**Licence.** WCS content may not be modified or adapted without the author's written permission. Never add resources here that inherit from, override or copy WCS resources.

## Testing

1. Subscribe to every dependency in the game. Run `Open WCS Workbench.cmd`; it refuses to start and lists the GUIDs of anything missing. The project GUID `1CAE49450A83DEAB` was written by hand; save the project once in Workbench.
2. Open the Chicken world and play. The log should show lines like `[AROR] Faction preset 'US_ARMY': N entries added from the faction catalogs` for US_ARMY, USSR_ARMY and FIA.
3. FIA should add only a few dozen items (WCS-made). If it adds hundreds, the excluded-source check (`$ArmaReforger:` / `$RHS_Core:` file lookups) is not working.
4. Raise Readiness with MARS and dispatch QRFs: the log shows `Ground package <profile> uses preset vehicle …` or `[AROR REAPER] package uses preset aircraft …`, or a `does not fit` warning with the authored fallback. Check that gunship escorts actually fire; REAPER seats only a pilot, so gunships whose weapons sit on a gunner turret may stay silent.
5. Open the Black Market with ports held: vehicles and firearms tabs should list WCS items after the curated ones. `[AROR GARRISON] … binding resolved with preset …` shows the garrison choices.
6. Without WCS (the normal core project), `entries added` lines should be absent or tiny: the base presets were imported from the base catalogs. A large number means the base presets are out of date; re-run the base import plugin.
