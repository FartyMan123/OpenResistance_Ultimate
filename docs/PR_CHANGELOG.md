# Add regional support, proximity garrisons and capture-driven QRFs

## Summary

Towns and cities now change control through support only. Military installations retain flag capture and gain proximity-activated defenders, with the intended flow: clear the site, capture its flag, then request the displaced hostile owner's QRF. Regional military ownership gradually influences settlement support through the existing support authority.

This also integrates paid QRF strategy, ground-response improvements and Resistance vehicle intelligence built on the commanders and staged waves from PR #11. Chicken retains 41 campaign locations: 18 support-only settlements and 23 military flags. Existing economy, support and location-control authorities remain responsible for campaign state.

Native tests demonstrate partial QRF delivery, Levie garrison deployment and successful wall grounding. User-reported tests at an outpost and relocated East Resources completed defender clearance, flag capture and subsequent QRF response; East Resources also confirmed garrison spawning. Other sites and the complete QRF lifecycle still need acceptance. This is an integration/draft PR.

## Changes

### Settlement support and military capture

- Remove poles from all 7 cities and 11 towns. Remove unused `Town_Flag`/`City_Flag` resources and the obsolete settlement-flag switch/API. Enforce support-majority control through the canonical settlement path and exclude settlements from military AI recapture.
- Add optional regional influence, disabled by default in both script and shared game-mode prefab; Chicken explicitly opts in. Group installations by their nearest settlement, with authored region-ID overrides. When enabled, periodically move support toward the region's share of military ownership through the existing normalization, majority, cooldown and persistence rules. The policy and rate remain provisional.
- Configure Chicken for capture-driven QRFs. Combat/damage requests have a separate `m_bCombatQRFs` opt-in, default off; killing or wounding a defender no longer creates a QRF request. Enabled military captures and support-driven settlement ownership changes can request the previous hostile owner's response.
- Distinguish physical hostiles from pending deployment and unsuitable terrain in replicated capture feedback. Bound the initial deployment window, including terrain/budget/resource/faction failures. Stop timed-out queue sources while preserving physical survivors, then let capture follow living hostiles. Unused slots are omitted for that activation and cannot appear late. The server still checks living hostiles and capture eligibility.

### Proximity garrisons and defence presets

- Add server-owned military defence rosters: infantry and manned MGs at ordinary outposts; parked unmanned armour and random crewed vehicles at military outposts; bounded random defence levels at resources, factories and ports; and the largest recipe at airbases. Settlements and HQs are excluded.
- Activate within 1 km of a living player, including direct teleport arrivals. Cache registry/component references, scan player proximity once per configured 1–2 seconds, admit one hull or team request per 250 ms globally, and use the native personnel queue with 6 m local player clearance (3 m for crew staging beside the flag).
- Reserve shared personnel capacity and virtualise only after all players remain beyond the wider retirement distance for the grace interval. Save logical recipes, casualties and coarse health; preserve dormant reserves and detach player/captured assets from cleanup. Use game-clock deadlines, and restore surviving crew from lost hulls on foot.
- Use compact flag-centred infantry sectors, outward MG arcs and vehicle pads. Check the complete terrain footprint and individual pads for water, slope, clearance and navigation. Bound local position repair and failed-placement retries; omitted unqueued positions are session-local placement outcomes, not casualties.
- Add eleven reusable sandbag-perimeter prefabs, including compact and coastal variants, with entrances and editable defence markers. Walls are scenario geometry; personnel, weapons and vehicles use proximity activation.
- Bind all 23 military sites to measured perimeter placements, including the final East Resources clearing. The airport anchor stays fixed. Authorised factory/resource moves apply only to Chicken; Rowan scene files remain untouched. Full audits reserve proposed footprints to avoid conflicting suggestions.
- Remove the redundant `MilitaryOutpost` location alias; Outpost and MilitaryBase directly inherit Location_Base with their own capture/garrison configuration. Preserve existing composition names, child IDs and measured wall transforms.
- Correct wall hierarchy/tagging and bound model grounding. Adjust standard cardinal infantry positions to meet MG clearance; custom compact pads retain infantry, MG and vehicle roles. Layout marker edits invalidate stale saved positions without replenishing losses.

### QRF funding, strategy and ground execution

- Price complete responses, reserve collateral, charge deployment overhead once and debit confirmed individual losses through the existing faction bank. Retain unpaid liabilities without negative balances or manufactured refunds. Save war pressure and debt through a versioned serializer.
- Maintain independent enemy war pressure, prioritize bounded/coalesced threats and select affordable response tiers. Author five ground tiers per opposing role while retaining role gates, cooldowns, capacity and staged-wave allocations. Invader dispatch and flight dispatch remain disabled in Chicken.
- Improve road launch-pad selection, direct seating verification, mounted ingress, physical arrival checks, unloading retries and stopped-vehicle passenger recovery. Preserve surviving infantry after hull loss and tolerate audited native blank vehicle affiliation without repainting vehicles.
- Support contact investigation, sector searches, a configurable security hold, bounded stalled-route recovery and return-based recovery/cleanup. Friendly outposts, military bases and airports can serve as strategic origins.
- Consolidate tactical helpers into the original executor component to resolve Workbench visibility issues; remove the obsolete tactical-executor script.

### Map intelligence, scenario data and tooling

- Add server observation and filtered owner-targeted last-seen vehicle contacts for Resistance maps. Contacts stop updating when unseen, fade and expire. Chicken currently enables temporary debug visibility of tracked vehicles; normal spotting remains available when disabled.
- Preserve all 41 stable location IDs, provisional settlement population of 1,230, existing Provins defenders and Rowan's airport anchor. Factory/resource relocation is authorised in Chicken; Rowan's scene files remain untouched. Extend the CSV generator with preset/yaw bindings and per-site terrain allowances.
- Retain terrain/model-based pole heights and reachable capture contexts for military flags. Add read-only defence auditing, bounded relocation suggestions and undoable wall grounding alongside the existing flag tools. The full audit searches a dense 25m grid with per-site bounds: normally 800m, 1200m for East Resources. Delimited audit imports reject incomplete runs. Import saved wall transforms into source data; regeneration retains them for unchanged site poses and invalidates them after relocation/rotation.
- Harden garrison persistence with schema 3: current catalogue bindings and fresh layout validation replace saved prefab paths/transforms. Migrate versions 1/2, preserve unaffected records on partial corruption and quarantine unsupported headers without automatic regeneration.
- Replace duplicate component “Enabled” settings with purpose-specific attributes and update their scenario overrides. Complete garrison asset/manager attribute descriptions. Rowan's separate placement/admin changes remain untouched.
- Update architecture and its full AGENTS.md copy, developer guidance, resource checks and mocked regressions. CLAUDE.md is unchanged.

## Testing

### Offline checks

- Focused actual-source mocked checks cover garrisons/support/serialization/capture, QRF integration, layouts/terrain/rotated footprint separation editor audit orchestration and editor-reference invalidation during grounding. Audit-import fixtures also pass. Native services are mocked; these are not an Enforce compile or gameplay test.
- Catalogue validation passes for 41 locations and 23 military poles, including stable IDs, coordinate provenance, inherited capture configuration, generated layers, prefab GUIDs and QRF origin gates.
- All eleven generated defence presets and the 23-site review validate. Installed/local metadata checks pass for 22 garrison/shared references; existing QRF asset/configuration checks cover 25 references, force profiles and tier capacity.
- `git diff --check` passes.

### Native evidence (before the latest review corrections)

- Earlier QRF runs verified startup, capture-driven admission, direct seating and Occupier affiliations. An airport response drove from Farm, partially entered the objective, began the 900-second hold and recaptured it. Recorded overhead and personnel/vehicle loss debits were paid with zero outstanding debt.
- User-reported Levie QRF testing confirmed all three vehicles arrived, unloaded infantry and began searching. Full hold, return and cleanup were not confirmed.
- Earlier Levie Military testing confirmed infantry, an MG, parked armour and a crewed vehicle deployed, but exposed blocked placements and a capture lock. A subsequent user-reported test at an unnamed outpost successfully cleared all defenders, captured the flag and received a QRF afterward, with good pathing. This confirms the tested sequence at that site; full hold, return and cleanup were not reported.
- The latest full native audit passed all 23 current footprints with zero relocation suggestions, including East Resources at its applied clearing about 866m from the former flag. Its measured relief is 1.18671m. Subsequent user-reported Play testing at East Resources confirmed garrison spawning, defender clearance, flag capture and a successful QRF response. Full QRF delivery, hold, return and cleanup were not reported.
- Andres Beacon near Gillnet Pond uses a measured replacement clearing about 783m from the former flag. Its latest orientation changes from 45 to 135 degrees to clear an MG arc. Access and full Play remain unverified.
- Historical editor measurements covered 41 poles before settlement poles were removed. Those measurements establish model/terrain heights, not acceptance of the current compounds or vehicle access. User testing confirmed Levie's town pole is absent.

After the final East Resources move, native wall grounding completed all 1,526 walls with zero warnings. The world was saved and all measured wall positions were imported into source data. The subsequent audit passed every current footprint.

### Acceptance still required

- Compile/reload the PR-review corrections. Earlier native results above predate the new deployment lifecycle, persistence migration and prefab hierarchy. Exercise invalid resources/factions, stalled/partial queues, exhausted personnel capacity, orphan crew after reactivation and legacy/partially damaged saves. Confirm queue cancellation preserves survivors and prevents late actors; test native player/foreign-asset transfer protection.

- Repeat the clear/capture/QRF test across other military-site types and multiplayer. Confirm combat alone does not dispatch a QRF, pending native queues resolve and blocked unqueued positions cannot leave a permanent capture lock or spawn later in that activation.
- Broaden garrison and capture/QRF gameplay coverage across other military-site types. East Resources passed the reported sequence; all 23 current footprints have passed the native editor audit. Repeat the editor audit after future placement edits and preserve its evidence separately.
- Verify complete QRF multi-package delivery, unloading, investigation, full security hold, return and cleanup. Validate practical routes and launch/dismount/return points across the island.
- Test dedicated-server/multiclient replication, capture-feedback consistency, recipient filtering, non-debug spotting, roster/debt/war save-load and sustained combined garrison/QRF performance. Run an explicitly configured Invader test separately.

## Notes

- Extenders must replace removed Town_Flag/City_Flag resources with Town/City and MilitaryOutpost with MilitaryBase. The obsolete settlement-flag attribute/API is removed. Component switches are renamed to `m_bHasGarrison`, `m_bGarrisonsActive`, `m_bRegionalSupport`, `m_bExecuteOperations` and `m_bReportDamage`; update inherited prefab/world overrides. Garrison schema 3 migrates old records but requires matching updated scripts; review regional bindings, role assets and defence templates.
- Existing QRF integration also changes force-profile prices, operation-manager serialization, spotting RPCs and map captions. Integrations referencing the removed tactical-executor script must use the original executor component.
- Save support/control through their existing authorities; save garrison recipes/losses/coarse health and QRF debt/war pressure through their owning serializers. Active operations, threat queues, collateral claims and spotting contacts remain session-local. Exact garrison inventory/ammo/fuel and durable captured-asset persistence are not implemented.
- Keep the existing Economy authority for balances and income. Physical vehicle stock, production and resource-backed readiness remain future work. Garrison and QRF personnel budgets are separate and require combined performance acceptance.
- New prefab GUIDs are hand-authored and checked against sibling metadata; native resource discovery remains part of validation. The existing experimental REAPER load-time dependency remains, despite disabled flight dispatch, and must be resolved for a base-game-only release.
- The Workbench plugins are intentional editor tools. Keep the separate gitignored EnfusionMCP handler folder excluded and remove it before packing. Preserve coordinate-source attribution/license files and reconcile manual grounding/placement edits into their source data before regeneration.
