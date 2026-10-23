# Commander foundation

The game-mode prefab creates three server-owned `AROR_Commander` contexts:

| Role | Initial faction key | Control |
|---|---|---|
| Resistance | AROR_RESISTANCE | Players |
| Occupiers | AROR_OCCUPIERS | AI |
| Invaders | AROR_INVADERS | AI |

Set faction keys on the shared `AROR_FactionRolesComponent` and the evaluation interval on `AROR_CommanderManagerComponent` in an inherited game-mode prefab. Commanders read the role component directly from their game-mode entity before creating contexts, so component initialization order does not require its singleton to exist first. The role remains stable when its faction changes. Each key must be non-empty and different from the other two. The role component checks faction existence separately; commanders do not change diplomacy. Game Flow and Resistance spawning use the same role mapping. Keep player loadouts and force profiles consistent with it.

Everon_Chicken and original Everon inherit the same core AROR faction mapping and Resistance loadout. Chicken's profiles distinguish their campaign faction (`AROR_OCCUPIERS` / `AROR_INVADERS`) from their native asset faction (`US` / `USSR`). Standalone test defenders resolve their initial affiliation from the shared defending role. No parallel commander faction configuration is required.

The manager creates its contexts at server initialisation, then waits for the existing Game Flow transition gate to allow GAME. Enemy evaluations begin one configured interval after readiness. This follows the existing readiness check, including its unverified persistence timing; it does not independently certify successful restoration. Resistance receives no automatic evaluation.

The Operations component subscribes to both enemy commanders and can schedule attacks against configured location IDs or accept QRF requests from trusted server code. See [Operations](OPERATIONS.md) for setup and executor integration. Physical ground-force execution is optional; economy transactions, player-order UI and persistence serializers remain planned. Operation records are session-local.

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

1. Open the addon in Workbench, reload changed resources and compile in Script Editor. Confirm only one `AROR_EFactionRole` definition (under `Scripts/Game/Factions/`) and one commander manager, role component and scheduler on the game mode.
2. Play either Everon world and expect `Commanders initialised: Resistance=AROR_RESISTANCE, Occupiers=AROR_OCCUPIERS, Invaders=AROR_INVADERS; waiting for campaign`. Disable the existing Workbench auto-new-playthrough setting when checking the manual setup path. Before setup completes, `IsCampaignActive()` must be false.
3. Complete setup. Check for the single `Commanders active` log. In a temporary server strategy subscriber or debugger breakpoint, observe one Occupier and one Invader callback per configured interval, starting after that interval. Confirm no Resistance callback occurs. Remove temporary instrumentation afterward.
4. On a dedicated server with two clients, confirm only the server creates contexts and evaluates commanders. Clients must get null from the manager singleton; joining another client must not add commanders or evaluation cycles.
5. In a test prefab, swap Occupier/Invader keys on the shared role component. Confirm the reported commander bindings swap while role and control mode remain unchanged. Test an empty key, duplicate keys and an interval below one second separately: each must log a configuration error and leave the manager disabled.
6. Stop and replay the world. Confirm the singleton and timers are cleared and exactly one set is created on the next run. Load a saved campaign and verify activation against actual HQ/restoration readiness as part of the existing Game Flow persistence tests.

The optional ground executor is enabled on Everon_Chicken with verified resource selections and explicit launch positions. The current override requests three US mechanized LAV packages; clear the Workbench profile/level overrides for randomized selection. See [executor setup](OPERATION_EXECUTOR.md). Territory outcomes and resources still come through their owning systems rather than the commander manager.
