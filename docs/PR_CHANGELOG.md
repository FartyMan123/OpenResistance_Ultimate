# Add regional support, proximity garrisons and capture-driven QRFs

## Summary

Towns and cities now change control through support only. Military installations retain flag capture and gain proximity-activated defenders, with the intended flow: clear the site, capture its flag, then request the displaced hostile owner's QRF. Regional military ownership gradually influences settlement support through the existing support authority.

This also integrates paid QRF strategy, ground-response improvements and Resistance vehicle intelligence built on the commanders and staged waves from PR #11. Chicken retains 41 campaign locations: 18 support-only settlements and 23 military flags. Existing economy, support and location-control authorities remain responsible for campaign state.

The latest work separates war level (QRF equipment pool) from aggression (response intensity 1–5), alongside fixes for empty capacity-blocked garrisons and objective-road planning. Chicken now allows a minimum tier-1 capture response without collateral when no paid recipe fits; overhead and confirmed losses remain payable through the existing bank/debt. Initial garrisons remain independent of bank funds. The current saved authoring is preserved, with four catalogue pose differences recorded for reconciliation. These source changes have offline coverage and require fresh Workbench compilation and acceptance; see [GAMEPLAY_READINESS.md](GAMEPLAY_READINESS.md).

This snapshot also includes the user's saved Chotain castle and a first read-only factory/resource cover survey. Further resource relocation and outpost fortification development were stopped before adopting new placements or adding new fortification geometry.

Native tests demonstrate partial QRF delivery, Levie garrison deployment and successful wall grounding. User-reported tests at an outpost and relocated East Resources completed defender clearance, flag capture and subsequent QRF response; East Resources also confirmed garrison spawning. Other sites and the complete QRF lifecycle still need acceptance. This is an integration/draft PR.

## Changes

### Independent war level and aggression

- Keep independent US/Occupier and USSR/Invader war progress and aggression. Captures slowly advance war progress and raise aggression more quickly; combat/confirmed losses affect aggression, and quiet time cools aggression without removing equipment progress. Capture remains the QRF dispatch trigger.
- Gate light, mechanized and combined ground pools by war level, with five aggression responses in each pool. Add verified M2 Humvee and PKM UAZ crew-only escorts. Enforce both recipe war ranges and profile unlocks, freeze equipment/intensity at admission, and retain affordability/capacity downgrades and staged reservations.
- Make the zero-funds level-1 exception respect its war pool. Retain bank/debt accounting and safe origin/cooldown limits. Keep aircraft dispatch disabled; the experimental paid air path also respects equipment unlocks.
- Save both axes with operation-manager schema 2, migrating schema 1 without resetting campaign progress. Include pending confirmed loss aggression/debt in read-only snapshots. Update RebIntel to display response level and independent aggression percentage; Invader backing/ambition reads the separate values.
- Classify the new escorts as light armed vehicles in Resistance intelligence, retaining ground spotting range and owner-targeted filtering. Update installed-resource checks, every faction/war/aggression combination and the price sheet. Native acceptance remains pending.

### Latest gameplay reliability pass

- Count submitted/physical garrison personnel commitments and admit complete teams/assets incrementally. Rotate nearby sites, bound empty-site capacity waiting and close established capacity-limited deployments before clearance so freed capacity cannot create late reinforcements.
- Permit earlier retirement under personnel pressure only after continuous all-player/site/actor separation without recent combat. Preserve casualties, damage and captured/player assets.
- Opt Chicken into individually validated infantry fallback when a full compound footprint fails. Omit unspawned heavy assets for that activation; keep navigation, collision, land, clearance and capture-area gates.
- Add optional rifleman/automatic-rifleman/grenadier/leader variants and bounded local infantry patrol/contact goals. Preserve old logical rosters and mounted MG/vehicle tasks. Damage reporting can update contact/war evidence while combat-triggered QRF dispatch stays off.
- Prefer paid QRFs; allow only a complete tier-1 capture response without collateral through an explicit Chicken opt-in. Charge its activation overhead and confirmed losses normally or retain debt; keep observing terminal physical survivors. Grant no funds and retain role/cooldown/actor gates.
- Plan road stops before spawning hulls/groups. Add bounded nearby road dismount followed by physical infantry approach, target the actual military flag and retain physical objective entry for the security hold. Requeue explicit pre-activation capture route failures within original age/cooldown limits, avoiding rejected origins; preserve healthy active packages.
- Require a currently friendly land origin in Chicken instead of falling back to configured coordinates after territory loss. Restore Durras's intended initial USSR owner/name after the editor save; existing saved campaign control remains authoritative.
- Add a read-only audit/report for all 23 saved military roots, catalogue bindings and activation overlaps. Preserve the user's ordinary-outpost wall removal, grouped scene, moved/rotated sites and Chotain castle. Generator reconciliation remains unfinished; do not regenerate over authored content.

### Settlement support and military capture

- Remove poles from all 7 cities and 11 towns. Remove unused `Town_Flag`/`City_Flag` resources and the obsolete settlement-flag switch/API. Enforce support-majority control through the canonical settlement path and exclude settlements from military AI recapture.
- Add optional regional influence, disabled by default in both script and shared game-mode prefab; Chicken explicitly opts in. Group installations by their nearest settlement, with authored region-ID overrides. When enabled, periodically move support toward the region's share of military ownership through the existing normalization, majority, cooldown and persistence rules. The policy and rate remain provisional.
- Configure Chicken for capture-driven QRFs. Combat/damage requests have a separate `m_bCombatQRFs` opt-in, default off; killing or wounding a defender no longer creates a QRF request. Enabled military captures and support-driven settlement ownership changes can request the previous hostile owner's response.
- Distinguish physical hostiles from pending deployment and unsuitable terrain in replicated capture feedback. Bound the initial deployment window, including terrain/budget/resource/faction failures. Stop timed-out queue sources while preserving physical survivors, then let capture follow living hostiles. Unused slots are omitted for that activation and cannot appear late. The server still checks living hostiles and capture eligibility.

### Proximity garrisons and defence presets

- Add server-owned military defence rosters: infantry and manned MGs at ordinary outposts; parked unmanned armour and random crewed vehicles at military outposts; bounded random defence levels at resources, factories and ports; and the largest recipe at airbases. Settlements and HQs are excluded.
- Activate within 1 km of a living player, including direct teleport arrivals. Cache registry/component references, scan player proximity once per configured 1–2 seconds, admit one hull or team request per 250 ms globally, and use the native personnel queue with 6 m local player clearance (3 m for crew staging beside the flag).
- Commit shared personnel capacity for submitted queues and physical assets; virtualise only after all players remain beyond the configured retirement distance for its grace interval. Save logical recipes, casualties and coarse health; preserve dormant reserves and detach player/captured assets from cleanup. Use game-clock deadlines, and restore surviving crew from lost hulls on foot.
- Register crew vehicles before boarding, verify physical occupancy and keep checking empty driver/gunner seats after initial readiness. Seat available crew while other members are queued; exclude players, foreign actors and destroyed hulls. Mounted crews receive no infantry defend waypoint, while lost-hull survivors defend on foot. The separate parked armoured vehicle remains intentionally unmanned.
- Use compact flag-centred infantry sectors, outward MG arcs and vehicle pads. Check the complete terrain footprint and individual pads for water, slope, clearance and navigation. Bound local position repair and failed-placement retries; omitted unqueued positions are session-local placement outcomes, not casualties.
- Add eleven reusable sandbag-perimeter prefabs, including compact and coastal variants, with entrances and editable defence markers. Walls are scenario geometry; personnel, weapons and vehicles use proximity activation.
- Bind all 23 military sites to measured perimeter placements, including the final East Resources clearing. The airport anchor stays fixed. Authorised factory/resource moves apply only to Chicken; Rowan scene files remain untouched. Full audits reserve proposed footprints to avoid conflicting suggestions.
- Remove the redundant `MilitaryOutpost` location alias; Outpost and MilitaryBase directly inherit Location_Base with their own capture/garrison configuration. Preserve existing composition names, child IDs and measured wall transforms.
- Correct wall hierarchy/tagging and bound model grounding. Adjust standard cardinal infantry positions to meet MG clearance; custom compact pads retain infantry, MG and vehicle roles. Layout marker edits invalidate stale saved positions without replenishing losses.

### QRF funding, strategy and ground execution

- Price complete responses, reserve collateral, charge deployment overhead once and debit confirmed individual losses through the existing faction bank. Retain unpaid liabilities without negative balances or manufactured refunds. Save war progress, aggression and debt through a versioned serializer.
- Maintain independent enemy progression/aggression, prioritize bounded/coalesced threats and select affordable response intensities. Author five ground responses in each equipment band per opposing role while retaining role gates, cooldowns, capacity and staged-wave allocations. Enable Invader ground capture responses in Chicken; retain the shared admission default off and keep combat/flight dispatch disabled.
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

### RebIntel phone app

- Add a portrait RebIntel home-screen tile and separate US/Occupier and USSR/Invader overview cards. Show War Level 1–5, aggression 0–100% with bars and configured response-policy status, including scenario-dependent Invader response admission.
- Replicate both roles' war progress and independent aggression with summary initialization. Refresh the open app once per second, use placeholders until data arrives and honour alternate faction bindings. The app remains read-only; the server operation manager owns the new aggression state.

### Invader territorial campaign

- Opt Chicken into distant resource-funded US/USSR engagements through the existing enemy commander evaluations; default the shared component off. Add a fixed `USSR SUPPORT CORRIDOR` offshore in the southeast and make the existing Durras Hill compound the initial USSR forward base, preserving its stable ID and measured placement. The corridor adds one non-capturable marker outside the 41-site catalogue.
- Advance along friendly nearby military sites, prioritising proximity to the corridor; use US counterattacks and a soft 40% installation-share target. Equal funding/industry gives 50/50 engagements. Held resources/factories affect odds; increased Invader war level/aggression raises ambition and support. Defer near players, physical/queued garrisons and active/pending QRF interests. Protect Resistance control and canonical cooldowns.
- Grant opening funds once through Economy and spend engagement costs from unreserved faction funds. Quote escalation support to the existing Income payout path. Save grant receipt/quarantine/cadence separately from Economy balances and location ownership. Suppress physical counter-QRFs for AI-versus-AI ownership changes while the campaign is active; enable player-capture ground responses for Invaders in Chicken while preserving the shared off default. RebIntel identifies the active territorial campaign.
- Record pressure when Resistance captures an Invader site even while physical Invader responses are disabled. Simulated AI-versus-AI captures do not escalate their own war pressure or create a funding feedback loop.

### Saved authoring and cover survey — unfinished

- Retain the user's saved Chotain castle and Workbench scene serialization. The castle's manual mesh children remain in MilitaryInstallations.layer.
- Add `Find covered factory and resource sites`, a read-only editor survey that ranks surrounding physical/visibility screening before checking terrain, pads, gun arcs and neighbour separation. Native compilation and survey execution are unverified; no candidates have been adopted.
- Catalogue regeneration and saved-wall parsing still require reconciliation with the hand-authored castle and grouped Workbench scene. Do not regenerate these layers over the castle until authored-subtree preservation is implemented. The interrupted preservation-tool patch was not applied.

## Testing

### Offline checks

- Focused actual-source mocked checks cover garrisons/support/serialization/capture, QRF integration, layouts/terrain/rotated footprint separation editor audit orchestration and editor-reference invalidation during grounding. Audit-import fixtures also pass. Native services are mocked; these are not an Enforce compile or gameplay test.
- The latest actual-source suites pass 11,932 mocked assertions, including 170 garrison/support checks, 37 crew/local-defence checks and 3,987 progression checks, crowded delayed-queue admission, minimum-response debt/loss accounting, bounded planning retries and pre-construction road planning. Native crew seating and sustained mounted combat still require Workbench acceptance.
- RebIntel mocked assertions cover role separation, server-only pressure changes, restoration, bounded aggression, unavailable data, disabled responses, faction labels, frame-container hierarchy, layout bindings, app navigation and live card/bar updates. Existing QRF strategy regressions also pass. Native phone opening initially exposed unsupported image parents; those were replaced with frames and separate background images.
- Invader actual-source mocked regressions cover resource spending/collateral, one-time grants, preparation/cadence restoration, player/Resistance protection, connected expansion and AI capture-response suppression. A funded offscreen model on the actual 23 military-site positions averages 39.7% Invader control at baseline and 63.7% at maximum escalation; these are policy-model results, not native income-balance acceptance.
- Catalogue validation passed for 41 locations and 23 military poles before the latest manual castle/world save. Generated-layer equality and saved-child parsing now require authoring reconciliation; the latest saved scene is retained intact.
- Earlier checks covered eleven generated defence presets and the 23-site review. Current installed/local metadata checks pass for 28 garrison/shared references and 27 QRF references, force profiles and tier capacity. Fourteen grounding parser fixtures and the read-only 23-root readiness audit pass. Current saved transforms differ from the catalogue at Pinewood Lake, Chotain Industrial, Old Wood/Wolf Hill and Andres Beacon; historical terrain checks do not validate those edits. Generated-prefab/world equality checks remain incompatible with intentional saved authoring. These checks do not accept the castle or unfinished cover survey.
- `git diff --check` passes.

### Native evidence (before the latest review corrections)

- User confirmed RebIntel works after correcting the card container hierarchy. Native value-change, restored-state and multiplayer acceptance remain outstanding. The new Invader territorial simulation has not been compiled or played in Workbench.

- Earlier QRF runs verified startup, capture-driven admission, direct seating and Occupier affiliations. An airport response drove from Farm, partially entered the objective, began the 900-second hold and recaptured it. Recorded overhead and personnel/vehicle loss debits were paid with zero outstanding debt.
- User-reported Levie QRF testing confirmed all three vehicles arrived, unloaded infantry and began searching. Full hold, return and cleanup were not confirmed.
- Earlier Levie Military testing confirmed infantry, an MG, parked armour and a crewed vehicle deployed, but exposed blocked placements and a capture lock. A subsequent user-reported test at an unnamed outpost successfully cleared all defenders, captured the flag and received a QRF afterward, with good pathing. This confirms the tested sequence at that site; full hold, return and cleanup were not reported.
- The latest full native audit passed all 23 current footprints with zero relocation suggestions, including East Resources at its applied clearing about 866m from the former flag. Its measured relief is 1.18671m. Subsequent user-reported Play testing at East Resources confirmed garrison spawning, defender clearance, flag capture and a successful QRF response. Full QRF delivery, hold, return and cleanup were not reported.
- Andres Beacon near Gillnet Pond uses a measured replacement clearing about 783m from the former flag. Its latest orientation changes from 45 to 135 degrees to clear an MG arc. Access and full Play remain unverified.
- Historical editor measurements covered 41 poles before settlement poles were removed. Those measurements establish model/terrain heights, not acceptance of the current compounds or vehicle access. User testing confirmed Levie's town pole is absent.

After the final East Resources move, native wall grounding completed all 1,526 walls with zero warnings. The world was saved and all measured wall positions were imported into source data. The subsequent audit passed every current footprint.

### Acceptance still required

- Follow the ordered [gameplay acceptance handoff](GAMEPLAY_READINESS.md): compile/reload, crowded/direct-teleport garrison activation, clear/capture without late spawns, paid/zero-funds capture QRFs, complete delivery/hold/return, then USSR, save/load and dedicated/multiclient checks. Repeat the native footprint audit on current saved authoring. Latest source corrections have no new native acceptance claim.

- Compile/reload the PR-review corrections. Earlier native results above predate the new deployment lifecycle, persistence migration and prefab hierarchy. Exercise invalid resources/factions, stalled/partial queues, exhausted personnel capacity, orphan crew after reactivation and legacy/partially damaged saves. Confirm queue cancellation preserves survivors and prevents late actors; test native player/foreign-asset transfer protection.
- Start fresh Play after compiling the crew correction. Verify the manned vehicle's driver/gunner and MG crews board, stay mounted and engage; check that surviving crew defend on foot after hull loss. Keep the deliberately parked armour empty.
- Open the phone with Ctrl+Y and select RebIntel. Check both cards, live changes after captures/quiet-time decay, Home/Esc navigation and Bank/landscape app switching. Verify server/client agreement, late joining and restored pressure in multiplayer; native UI rendering and replication are not yet accepted.
- Compile/reload the Invader campaign and start a fresh campaign. Verify the southeast corridor, USSR Durras garrison, single opening grant, distant captures/counterattacks and protected nearby/Resistance sites. Check ordinary Income escalation, save/load receipt/cadence, AI-only QRF suppression and client map/RebIntel state. Sustain a campaign with real income/QRF spending before accepting territorial balance.
- Chicken now enables USSR ground capture responses for native testing. Let Invaders hold a second military site before capturing one: the offshore corridor cannot launch vehicles, and Chicken has no authored safe Invader coordinate fallback. Verify Ural/BTR/BRDM crews, delivery, full hold, return and cleanup; enabling admission is not native lifecycle acceptance.

- Repeat the clear/capture/QRF test across other military-site types and multiplayer. Confirm combat alone does not dispatch a QRF, pending native queues resolve and blocked unqueued positions cannot leave a permanent capture lock or spawn later in that activation.
- Broaden garrison and capture/QRF gameplay coverage across other military-site types. East Resources passed the reported sequence; all 23 footprints passed an earlier native editor audit before the latest manual authoring. Repeat the editor audit on the saved scene and preserve its evidence separately.
- Verify complete QRF multi-package delivery, unloading, investigation, full security hold, return and cleanup. Validate practical routes and launch/dismount/return points across the island.
- Test dedicated-server/multiclient replication, capture-feedback consistency, recipient filtering, non-debug spotting, roster/debt/war save-load and sustained combined garrison/QRF performance. Complete the enabled Chicken Invader ground-response acceptance, including a second friendly land origin and concurrent Occupier capacity checks.

## Notes

- Extenders must replace removed Town_Flag/City_Flag resources with Town/City and MilitaryOutpost with MilitaryBase. The obsolete settlement-flag attribute/API is removed. Component switches are renamed to `m_bHasGarrison`, `m_bGarrisonsActive`, `m_bRegionalSupport`, `m_bExecuteOperations` and `m_bReportDamage`; update inherited prefab/world overrides. Garrison schema 3 migrates old records but requires matching updated scripts; review regional bindings, role assets and defence templates.
- Existing QRF integration also changes force-profile prices, operation-manager serialization, spotting RPCs and map captions. Integrations referencing the removed tactical-executor script must use the original executor component.
- RebIntel adds replicated war-summary properties to the operation manager; server and clients need matching scripts/layouts. Aggression now controls desired response intensity independently of war-level equipment availability. Operation-manager schema 2 reads schema 1 and preserves prior progress; server and clients require matching updated scripts.
- Invader campaign state has its own versioned serializer. `SUPPORT_CORRIDOR` is appended without changing existing enum values. Chicken's initial Durras owner/name changes; explicit saved control is retained when present. The corridor is an abstract offshore logistics marker; the land garrison uses the existing Durras compound, with no new terrain placement claim. Extenders must opt in and author corridor/base IDs; physical stock and production remain future work.
- Save support/control through their existing authorities; save garrison recipes/losses/coarse health and QRF debt/war pressure through their owning serializers. Active operations, threat queues, collateral claims and spotting contacts remain session-local. Exact garrison inventory/ammo/fuel and durable captured-asset persistence are not implemented.
- Keep the existing Economy authority for balances and income. Physical vehicle stock, production and resource-backed readiness remain future work. Garrison and QRF personnel budgets are separate and require combined performance acceptance.
- New profile/recipe extension points include minimum war unlocks, recipe war ranges and light-contact classification; readiness contexts supply war level and `Resolve` accepts it as an optional fifth argument. `DesiredTier` now takes an aggression level as its first argument; threat context no longer grants a tier bonus. New scenario extension points include minimum capture-response funding, friendly-origin enforcement, infantry-only terrain fallback, specialist variants and local defence patrol/capacity-wait settings. Shared minimum-funding/terrain exceptions remain off; Chicken explicitly opts in. Garrison serializers retain their formats; the operation manager moves to schema 2 for independent aggression. Deployment/retry/account state remains session-local.
- New prefab GUIDs are hand-authored and checked against sibling metadata; native resource discovery remains part of validation. The existing experimental REAPER load-time dependency remains, despite disabled flight dispatch, and must be resolved for a base-game-only release.
- The Workbench plugins are intentional editor tools. Keep the separate gitignored EnfusionMCP handler folder excluded and remove it before packing. Preserve coordinate-source attribution/license files and reconcile manual grounding/placement edits into their source data before regeneration.
