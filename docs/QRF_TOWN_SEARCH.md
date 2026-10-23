# Mounted QRF town search

Ground QRF packages can now perform a bounded mounted search before resuming their normal deployment. This source implementation has not been compiled or tested in Workbench yet. The previous live test verified spawning and boarding; it did not prove driving, and the newer navigation bindings also need a fresh run.

Each package drives to its initial approach point (armour approaches the objective), then `AROR_VehicleTownSearch` picks road points inside the attacked location. It samples different town sectors with varied radii, uses `RoadNetworkManager.GetReachableWaypointInRoad` to reject unusable candidates, and rejects stops outside the town or too close to the vehicle. Failed road queries have bounded retries; there is no off-road/random-coordinate fallback. Packages choose their routes independently rather than sharing a convoy route.

The search never queries player positions or continuously retargets to the shooter. It searches the reported town. Native AI perception, combat and waypoint behavior determine spotting and engagement; the coordinator does not add a custom target-acquisition or pursuit system. Chicken now includes the verified base-game PerceptionManager prefab alongside its AIWorld and navigation bindings, following Bohemia's [Everon example](https://github.com/BohemiaInteractive/Arma-Reforger-Samples/blob/main/SampleMod_NewFaction/Worlds/MP/CTI_Campaign_Eden_base_Layers/AI.layer).

## Settings

These attributes are on `AROR_OperationExecutorComponent`, with explicit settings in Chicken:

| Attribute | Default | Purpose |
|---|---|---|
| `m_bQRFVehicleSearch` | Enabled | Enable mounted searching for ground QRFs |
| `m_iQRFSearchDuration` | 180 seconds | Overall mounted-search limit |
| `m_iQRFSearchStops` | 6 | Maximum stops issued per package |
| `m_iQRFSearchDwell` | 8 seconds | Pause after physically reaching a stop |
| `m_iQRFSearchLegTimeout` | 60 seconds | Skip a stalled leg after this delay |

These are server timer ticks, so delays may lengthen under load. Scheduled attacks, standalone infantry and unsupported airborne profiles do not enter this search phase. The normal operation timeout remains authoritative and can terminate a search earlier.

At the duration/stop limit, or if no further reachable road point is available, the search removes its own waypoint and returns control to the existing deployment policy. Motorized and mechanized packages return to their dismount point and unload; mechanized crews then receive their normal support order. Armour returns to its mounted objective order. A missing initial search route leaves the original deployment order intact. Search arrival does not capture territory or declare victory.

Terminal operation cleanup removes the search waypoint before deleting package-owned groups and vehicles. A destroyed vehicle before delivery still fails that package. Dormant groups retain their reservation; after they wake, the search may expire based on the original deadline. Existing preservation rules apply to player-controlled entities and occupied vehicles.

## Runtime acceptance

1. Recompile and reload `AROR_Everon_Chicken` from disk before saving. Confirm soldier/vehicle navmeshes load without empty-navmesh warnings and the PerceptionManager loads.
2. Complete campaign setup and damage a US test defender as a hostile faction. The level-one motorized fixture should spawn, board and become ACTIVE.
3. At the approach point, expect `Mounted town search started` and a road-point log. Observe actual movement to the stop and `reached stop`; ACTIVE or a waypoint-issued log alone does not prove movement.
4. Hide or move elsewhere. Confirm road stops remain town searches, rather than following the player through walls or outside the town. Native AI may react if it detects an enemy.
5. Observe the stop/duration limit, return to deployment, passenger exits and infantry objective orders. Test an unavailable/stalled road and verify skip/fallback rather than endless waypoint creation.
6. Cancel or destroy the force during a search; verify no search waypoint, unowned troops or abandoned operation-owned vehicle remains. Repeat with mechanized and armour profiles and with Invaders in separate sessions.
7. Disable `m_bQRFVehicleSearch` and confirm direct deployment resumes. Scheduled attacks should retain direct deployment with either setting.

The road API and its parameter order were checked against the installed official API reference and Bohemia's [resupply task solver](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff/blob/3d77cc212d5cda9922daf5f45635c7300d2d4cce/scripts/Game/TaskSolver/SCR_ResupplyTaskSolver.c). A successful query does not establish that every modded vehicle can navigate every chosen road; real routes and combat interruptions remain engine tests.
