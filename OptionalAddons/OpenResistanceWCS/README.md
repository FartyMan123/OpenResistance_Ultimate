# Open Resistance Ultimate - WCS (dependency pack)

An empty project whose only job is to depend on the core and the Worst Case Scenario (WCS) content addons. It holds no scripts or configs. It serves two purposes:

- **Workbench testing:** opened through its launcher, it loads WCS together with the repository core.
- **Hosting from the game:** published to the Workshop, enabling this one item pulls in Open Resistance Ultimate, RHS: Status Quo and every WCS addon below, so an in-game host needs nothing else. Dedicated servers can use it the same way, or list the addons themselves (`server/AROR_Everon_Chicken.wcs.json`).

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
| Black Market and Imports | Every WCS item (Black Market) and vehicle, helicopters and tanks included (Imports). Each added entry copies the price and import facility requirements of a curated entry of the same type, e.g. attack helicopters 60,000 with 6 facilities, one of them a heliport or airport; IFVs (22,000, 5 + seaport) and tanks (40,000, 6 + seaport) use defaults |
| Arsenal (admin fill) | The WCS-made firearms in the FIA preset |

## Dependencies

The core (`6C68A5E5654D7210`), RHS: Status Quo, and the WCS content addons: Core, Arsenal, Armaments, Weapons, Attachments, Scopes, Clothing, NATO, RU, RHS Weapons/Attachments/Scopes/Vehicles, Vehicles, Stryker, FMTV, JLTV (+ Upgrade), MRZR, Motorbikes, ZU-23-2, KBA-105, Helicopters, AH-1S, AH-64D (+ Upgrade), AH-6M, Mi-24V (+ Upgrade) and KA-52 (+ Upgrade). WCS server-gameplay addons and maps are deliberately left out.

**Licence.** WCS content may not be modified or adapted without the author's written permission. Never add resources here that inherit from, override or copy WCS resources.

## Testing

1. Subscribe to every dependency in the game. Run `Open WCS Workbench.cmd`; it validates the complete dependency chain, including engine data, and refuses to start for a missing GUID, dependency cycle or ambiguous installed copy. Folder names alone are not trusted. The project GUID `1CAE49450A83DEAB` was written by hand; save the project once in Workbench.
2. Open the Chicken world and play. The log should show lines like `[AROR] Faction preset 'US_ARMY': N entries added from the faction catalogs` for US_ARMY, USSR_ARMY and FIA.
3. FIA should add only a few dozen items (WCS-made). If it adds hundreds, the excluded-source check (`$ArmaReforger:` / `$RHS_Core:` file lookups) is not working.
4. Raise Readiness with MARS and dispatch QRFs: the log shows `Ground package <profile> uses preset vehicle …` or `[AROR REAPER] package uses preset aircraft …`, or a `does not fit` warning with the authored fallback. The current REAPER adapter seats one pilot per aircraft. Preset gunships requiring additional turret crew are rejected with an explanation and fall back to the authored gunship. Test firing on compatible pilot-controlled aircraft in Workbench; this is not proof of weapon/AI compatibility for every WCS variant.
5. Capture import facilities (the admin menu's capture on map is quickest): Black Market should list WCS weapons (search e.g. "SCAR") and Imports WCS vehicles and helicopters, priced like the curated ones (no 0-cost entries). Helicopters stay locked without a heliport or airport. `[AROR GARRISON] … binding resolved with preset …` shows the garrison choices.
6. Without WCS (the normal core project), `entries added` lines should be absent or tiny: the base presets were imported from the base catalogs. A large number means the base presets are out of date; re-run the base import plugin.

## Hosting a WCS server

`server/AROR_Everon_Chicken.wcs.json` is a dedicated-server config for the Chicken world (`Missions/AROR_Everon_Chicken.conf`), listing the core, REAPER, RHS: Status Quo and every WCS addon above. The server and clients download them all from the Workshop, so the Workshop build of the core must contain this mission header and the catalog augmentation code; an older core build fails to find the scenario or runs without WCS assets. The mission header's GUID was written by hand; save it once in Workbench.

Before publishing, use Workbench's multiplayer peer tool from the WCS test project to check the server/client split locally.

## Review and repeatable checks

See [WCS integration review](../../docs/WCS_REVIEW.md) for the verified dependency/asset scope, fixes and remaining native checks.

- `powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Start-WCSWorkbench.ps1 -PrintOnly` validates the actual installed dependency chain without launching Workbench.
- `powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Test-WCSDependencies.ps1` runs isolated missing/transitive/misidentified/duplicate/cyclic dependency fixtures.
- `node tools/Test-WCSIntegration.js` exercises the actual classification and vehicle/aircraft fit decisions with mocked native services.
- `python tools/Test-WCSAssets.py --game-addons "<downloaded addons folder>" --base-addons "<Arma Reforger installation>/addons"` checks the reviewed preset/profile asset paths and GUID metadata against installed packages without editing them.

The shared classifier identifies T-14/T-72/M1 variants before broad APC/truck labels, separates static anti-air and anti-tank weapons from machine guns, keeps armed trucks out of troop/supply-truck pools, and recognises anti-air launchers. These rules affect generated entries; authored preset entries retain their own types and requirements. Authored garrison indices are preserved; existing saves containing formerly misclassified modded variants require a restore check because generated pool membership changes.
