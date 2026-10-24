# Truck and IFV combined QRF test

The [USSR/Invader framework](INVADER_QRF_FRAMEWORK.md) is staged with levels 1-5 and operation admission disabled; the current US/Occupier test remains selected.

Chicken now selects `combined_arms_l1` as an explicit ground QRF recipe. The helicopter override is disabled for this test. This is authored tier 1 with **two elements**, not two copies of a profile and not a level-2 response.

| Element | Occupiers fixture | Personnel |
|---|---|---|
| Motorized | M923A1 transport truck | One driver and four-person US fireteam |
| Mechanized | LAV-25 IFV | Three crew and four-person US fireteam |
| Total | Two vehicles, four groups | Twelve actual personnel; eighteen reserved slots |

Both use the configured `AROR_OCCUPIERS` faction. An equivalent Invaders binding resolves the existing Ural/BTR profiles to `AROR_INVADERS`. Crew requirements come from those concrete profiles; the roster CSV's nominal crew estimates do not override them.

The entire response reserves four groups, eighteen personnel slots and two vehicles before any members enter the engine queue. Both elements must be ready/boarded before activation. A required element lost during spawning fails and cleans up the undispatched response. After activation, losing one package leaves the other operating. The new [shared tactical commander](QRF_TACTICAL_COMMANDER.md) retains nearby terminal forces and their capacity until age AND all-player distance permit cleanup.

Tactical coordination is now enabled: the truck delivers infantry and waits; the IFV delivers infantry and covers a separate reachable road sector. Troops exit before investigating fresh contact and searching separate town sectors. Contact changes, new waves and losses can retask surviving packages at a bounded interval. Launch spacing remains 35 m, now lateral around the selected origin. The nearest eligible friendly outpost remains the origin when available, otherwise configured role coordinates are used. Convoy driving and matching arrival times are not guaranteed. This two-element tier-1 fixture stays in one wave; larger responses can use the new deferred reserve policy.

## Run

1. Stop the old Play session, compile Game scripts successfully, then reload `AROR_Everon_Chicken` from disk.
2. Start Play, complete campaign setup and attack the existing assigned US defender in Provins.
3. Look for `Composite QRF ... recipe=combined_arms_l1 tier=1 elements=2 reserved groups=4 personnel=18 vehicles=2`.
4. Expect one `Ground package queued` line for `us_motorized` and one for `us_mechanized` under the same operation ID. Both packages should board and the operation should reach ACTIVE (`state=2`).
5. Let both vehicles approach and infantry exit before destroying them. Check the truck and IFV use their different deployment policies and infantry search sectors. Then test one active vehicle loss: the surviving element must continue. Restart and test cancellation or a required vehicle loss before activation: no partial response should dispatch and all reservations should release.

To restore the previous helicopter experiment, clear **Ground QRF Recipe Id**, enable **Reaper QRF Test**, and set **Workbench Response Level** to the desired air level. An explicit ground recipe takes priority if both modes are configured. Clearing the ground recipe with REAPER off restores the existing homogeneous profile selector; the previous forced US-only profile is now cleared.

## Configuration and status

`AROR_GroundQRFRecipe` references ordered existing profile IDs and an authored level. Repeating an ID deliberately adds another complete element. Profiles must be enabled, QRF-eligible, ground-supported and match the recipe's role/faction. Recipe IDs may be reused across roles, but duplicate ID/role pairs are rejected. One to eight elements and tiers 1-5 are supported; the selected list is deployed once.

The operation records recipe identity, response tier and expected package count separately. Recipe/profile bindings are mutually exclusive and cannot reroll. Shared budgets include other active/reserved/retained forces. Unsupported airborne elements fail validation; mixed ground/air recipes and resource-aware selection remain future work. There is no campaign spending or stock reservation in this prototype.

Earlier logs verified mixed spawning, boarding, correct campaign factions and activation. The new tactical/wave source and updated Workbench smoke tests are uncompiled/unrun in the engine. Follow the acceptance checks above and the tactical guide before treating revised delivery, search, withdrawal, retention or performance as verified.
