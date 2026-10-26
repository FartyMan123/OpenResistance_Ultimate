# Fortify Everon outposts and improve HICOMM orders

## Summary

Give Chicken's ordinary and military outposts stronger defensive layouts, varied crewed weapons and equipment stores. Improve HICOMM order controls and make attacking squads travel to their objective before beginning an assault.

## Changes

### Fortified outposts and garrisons

- Bind equipped compositions to 14 ordinary and four military outposts, preserving Chotain castle, stable location IDs, owners and flag/root transforms.
- Add sandbag perimeters, gun revetments, infantry sectors, command shelters/desks and entrance lanes. Seventeen sites use compact footprints of 28–48 m; Hospital retains its 64 m layout.
- Add four native equipment containers per equipped site: weapons, ammunition, equipment and medical supplies (72 total), with US/USSR wrappers and server-side inventory observation.
- Configure two crewed statics for ordinary sites and four for military sites, mixing heavy/medium MG variants. Compact military recipes keep the manned vehicle and omit the spare uncrewed hull.
- Preserve saved roster losses and variants; revised recipes apply to fresh rosters.

### HICOMM and High Command

- Default to MOVE; left-click plans the selected order and right-click issues it. ATTACK/DEFEND select a nearby location and report when no target is available.
- Send attacking squads to the objective first, switch to assault on arrival, then defend after the attack waypoint completes.
- Show HOLD, ALL MINE and NONE selection states, reposition zoom controls around the slider and remove the duplicate enemy war summary from the top bar.
- Add experimental embedded terrain-map support and isolate its map icons/lifecycle. The terrain view remains disabled by default; the procedural tactical map remains active.

### Editor tools and documentation

- Add an undoable fortified-outpost grounding command that preserves Chotain castle.
- Add a current-placement audit with blocked-pad, wall-segment and gun-arc diagnostics; retain the bounded relocation-search command.
- Make audits respect authored defence levels, static counts and compact hull recipes.
- Archive the initial failed footprint audit, track current trial bindings and guard campaign-layer regeneration against overwriting authored compounds.
- Document configuration, extender impact and the native test sequence.

## Testing

- 313 actual-source mocked assertions passed for garrisons, crews, editor audits, grounding and supply observation.
- 1,549 estimated layout assertions passed for compact geometry, marker spacing, flag access, gun rays, container IDs and infantry/vehicle entrance routes.
- Installed-asset validation passed 53 native/local resource references; metadata checks found no duplicate resource GUIDs.
- All 23 military location IDs, 18 equipped bindings and 72 container bindings checked; protected site data retained.
- `git diff --check` passed.
- HICOMM's existing commit records Workbench Play testing, without full acceptance coverage.
- Final outpost scripts/resources have not been compiled or accepted in Workbench. The initial larger-layout audit rejected all 18 trial sites; the compact revisions still require grounding, a new audit and Play testing.

## Notes

- Target branch: `master-dev`.
- Existing saved garrisons retain their recipes and casualties. Use a fresh campaign to test the new counts and variants.
- Native container stock, looting/depletion, inventory persistence, crew seating, navigation, dedicated-server replication and performance remain acceptance work.
- Extenders should review `m_aStaticVariants`, `m_iStaticWeaponCountOverride`, `m_bOmitParkedArmour` and the new composition/supply component resources. Server and clients need matching scripts/resources. New resource GUIDs are hand-authored and still require Workbench discovery.
- No new economy or territory authority is introduced. HICOMM's existing session-only squad model remains.
- Before testing: compile and reload Chicken, run **Ground fortified outposts (preserve Chotain castle)**, inspect and Save, then run **Audit fortified outposts (current placements)**.
