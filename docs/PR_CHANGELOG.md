# Add OPS missions and fix mission lifecycle failures

## Summary

Add the OPS mission board and six Resistance side missions. Address [Rowantrek's review on PR #27](https://github.com/Rowantrek/AR-Open-Resistance/pull/27#pullrequestreview-5659870942): prevent false convoy completion, restore leader management after loading, validate waiting targets, roll back failed spawns and correct rescue/cache rewards.

## Changes

- **Missions / Phone:** six mission types, leader accept/abort controls, OPS dossiers and markers on the game map and HICOMM.
- **Convoy Ambush:** distinguish unspawned crew from casualties, skip empty unit prefabs and retry seating until surviving crews are aboard. Restored convoys wait for players near their origin before restarting.
- **Mission lifecycle:** let any current Resistance Leader abort restored missions and receive progress hints. Replicate the session owner ID so the OPS abort button follows the server's permissions.
- **Deployment:** cancel invalid targets before spawning; immediately remove partial deployments before retrying. Preserve the occupied-vehicle cleanup exemption.
- **Rewards:** wait for every surviving prisoner to arrive and count only those delivered; retain secured cache crates for looting.
- **Operations:** retain the existing wider QRF road-launch search and seating retries.
- **Documentation / checks:** update architecture, lifecycle rules and Workbench acceptance steps; add actual-source mocked regression checks.

## Testing

- 84 assertions passed using selected actual script method bodies with mocked native services, covering convoy activation/seating/casualties, prisoner arrivals, restored abort permissions and hints, and immediate deployment rollback.
- Workbench Script Editor compile passed (confirmed by the developer). World Editor Play and dedicated-server acceptance checks remain pending; see `docs/MISSIONS.md`.

## Notes

- Target branch: `master-dev`; review fixes are on `chicken-dev`.
- Existing version-1 mission saves remain compatible. Restored missions have shared leader management; session IDs are not saved.
- Extenders: mission snapshot rows now contain 22 fields, including the session owner ID; clients and server need matching scripts. Failed deployments delete tracked provisional entities/groups immediately rather than invoking `Release_S`. Track partial resources in the shared arrays; use `Release_S` for completion rewards and cleanup exemptions.
- Native AI activation, seating, navigation, loot retention and multiplayer replication still require Workbench acceptance.
