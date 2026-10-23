# Commander foundation

The game-mode prefab creates three server-owned `AROR_Commander` contexts:

| Role | Initial faction key | Control |
|---|---|---|
| Resistance | FIA | Players |
| Occupiers | US | AI |
| Invaders | USSR | AI |

Set the faction keys and evaluation interval on `AROR_CommanderManagerComponent` in an inherited game-mode prefab. The role remains stable when its faction changes. Each key must be non-empty and different from the other two. Referenced factions must also exist in the scenario's faction manager; this foundation does not verify faction existence or change diplomacy. When changing Resistance, also update the existing HQ/player faction and loadout configuration.

The manager creates its contexts at server initialisation, then waits for the existing Game Flow transition gate to allow GAME. Enemy evaluations begin one configured interval after readiness. This follows the existing readiness check, including its unverified persistence timing; it does not independently certify successful restoration. Resistance receives no automatic evaluation.

The Operations component now subscribes to both enemy commanders and can schedule attacks against configured location IDs or accept QRF requests from trusted server code. See [Operations](OPERATIONS.md) for setup and executor integration. There are no physical forces, resource transactions, player-order UI or persistence serializers yet. Operation records are session-local.

## Strategy integration

Server systems obtain the manager with `AROR_CommanderManagerComponent.GetInstance()`. It returns null on clients. Use `GetCommander(AROR_EFactionRole.OCCUPIERS)` to query a context. Unknown roles return null.

Subscribe using `GetOnCommanderEvaluation().Insert(OnCommanderEvaluation)` after the manager exists, and remove that subscription when the consumer is deleted. The handler signature is:

```c
void OnCommanderEvaluation(AROR_Commander commander)
{
	// Select an affordable operation for commander.GetRole().
	// Keep location ownership, resource balances and spawned groups in their own systems.
}
```

The event is synchronous and server-only. Keep handlers short; submit operations to a separate executor rather than running combat or expensive world scans inside the callback. Each handler receives the Occupier context and then the Invader context per cycle. Player actions will need a separate, authorised server request path.

## Workbench acceptance checks

These checks require Arma Reforger Tools; they have not been run by Codex.

1. Open the addon in Workbench, let it discover the new scripts, and compile in Script Editor. Inspect the base game-mode prefab and confirm it has exactly one commander manager. The new prefab component has no hand-written instance ID; let Workbench assign one if it requires one when saving. No resource GUIDs or `.meta` files were fabricated.
2. Play Everon and check the server log for `Commanders initialised: Resistance=FIA, Occupiers=US, Invaders=USSR; waiting for campaign`. Disable the existing Workbench auto-new-playthrough setting when checking the manual setup path. Before setup completes, `IsCampaignActive()` must be false.
3. Complete setup. Check for the single `Commanders active` log. In a temporary server strategy subscriber or debugger breakpoint, observe one Occupier and one Invader callback per configured interval, starting after that interval. Confirm no Resistance callback occurs. Remove temporary instrumentation afterward.
4. On a dedicated server with two clients, confirm only the server creates contexts and evaluates commanders. Clients must get null from the manager singleton; joining another client must not add commanders or evaluation cycles.
5. In a test prefab, swap Occupier/Invader keys. Confirm the reported faction bindings swap while role and control mode remain unchanged. Test an empty key, duplicate keys and an interval below one second separately: each must log a configuration error and leave the manager disabled.
6. Stop and replay the world. Confirm the singleton and timers are cleared and exactly one set is created on the next run. Load a saved campaign and verify activation against actual HQ/restoration readiness as part of the existing Game Flow persistence tests.

An optional infantry executor and Workbench QRF test controls are now placed on Everon_Chicken's game-mode instance. The executor starts disabled and needs valid group/waypoint resource selections and launch positions. See [executor setup](OPERATION_EXECUTOR.md). Territory outcomes, real combat reports and resources still come through their owning systems rather than the commander manager.
