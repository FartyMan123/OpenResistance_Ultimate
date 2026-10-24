# QRF tactical commander and staged waves

Status: implemented in source, **not yet compiled or Play-tested in Workbench**. The current Chicken fixture enables it for the Occupier truck/LAV response. Invader operation admission remains OFF; no USSR trigger, territory, factory or resource production has been added.

## Responsibility and faction ownership

One server-owned `AROR_QRFTacticalCommander` belongs to each dispatched QRF operation. It coordinates the selected force after the strategic commander has requested a response. It uses the existing operation manager, shared role bindings, executor reservations, native AI groups and REAPER adapter. It does not create another faction, territory or economy authority.

| Role | Campaign faction | Current assets | Dispatch state in Chicken |
|---|---|---|---|
| Occupiers / BLUFOR | `AROR_OCCUPIERS` | US truck, LAV and UH-1 profiles | Existing Provins defender test |
| Invaders / REDFOR | `AROR_INVADERS` | USSR Ural, BTR, BRDM and Mi-8 profiles | Admission disabled; five ground tiers staged |

The same tactical code handles both roles. Asset selection stays in the existing role/faction profiles and recipes. Unit, group and vehicle faction audits still apply. The Resistance remains player-directed.

Strategic/resource selection and tactical execution have separate responsibilities: the readiness-provider preview can later choose a feasible recipe, while this commander executes the immutable recipe/profile and tier that the operation already selected. No resource spending, stock regeneration or automatic preview caller is connected here.

## Response lifecycle

1. Validate and bind the complete response once. Reserve its entire group/personnel/vehicle budget before spawning actors.
2. Freeze the launch origin from the nearest eligible friendly outpost, or the existing configured role position when no eligible outpost exists.
3. Partition eligible large responses into a lead wave and finite reserve. Ground lead selection includes motorized, mechanized and armour elements where the recipe provides them.
4. Construct at most one ground element **or** one transport/gunship pair per executor tick. All required lead elements must finish spawning/boarding before the parent becomes ACTIVE.
5. Approach separate road points, deliver infantry, investigate reported contact and search assigned sectors. Protected vehicles cover separately; gunships retain their report-driven search/pass behavior.
6. Admit the original reserve elements after their delay, under live limits and existing aggregate reservations. Later waves join the same operation without a second ACTIVE transition.
7. Withdraw on task duration, severe casualties or ownership loss. Expire unused reserves rather than spawning them indefinitely.
8. Retain nearby physical forces/casualties after terminal handling. Release their capacity only through normal safe cleanup.

Pre-dispatch loss of a required lead element fails the whole undispatched response. Active losses degrade the response. A failed reserve construction does not cancel a healthy lead wave. Each authored element can be constructed only once; reserves are not an unlimited replacement pool.

## Two-wave policy

Defaults split responses at tier 3 or above **when there are at least four elements**. Lead size is the rounded-up half; the rest remains virtual and has no soldiers or vehicles yet. Small responses stay intact.

| Prepared ground response | Total elements | Lead | Reserve | Default live ground ceiling |
|---|---:|---:|---:|---:|
| Current US truck + LAV, tier 1 | 2 | 2 | 0 | 6 |
| Invader tier 1 | 2 | 2 | 0 | 6 |
| Invader tier 2 | 3 | 3 | 0 | 6 |
| Invader tier 3 | 5 | 3 | 2 | 6 |
| Invader tier 4 | 7 | 4 | 3 | 6 |
| Invader tier 5 | 8 | 4 | 4 | 6 |

The USSR rows describe prepared recipes, not enabled dispatch. Homogeneous US/USSR ground responses still use their selected level as total package count: level 4 splits 2/2 and level 5 splits 3/2. Recipe tiers remain distinct from element count.

For a tier-5 eight-element ground recipe, the lead contains at least one truck, IFV and armour element. At 60 seconds after first dispatch, reserve elements may construct until six live packages are present. The remainder waits for capacity. Loss of a package shortens the release delay to 15 seconds after first dispatch; it does not reset the timer or create a new allocation.

An air level-5 response remains five complete transport/fireteam/gunship packages. Three pairs dispatch first (six aircraft); two pairs remain virtual. A later pair requires **two** free live-aircraft slots. A successfully departed and removed transport can free one slot, but that alone cannot admit a pair. Healthy retained or disabled aircraft still occupy physical slots; a loss flag alone does not admit another pair. Delivered infantry remains owned and globally budgeted after its transport departs.

Waves may overlap under the live ceiling. Unspawned reserves expire 300 seconds after first dispatch. If survivors continue to occupy all slots, some authorized reserves can expire unused; this deliberately favors the cap over forcing every element onto the map. Expired reserves release only their unspawned allocation. They do not delete the lead force.

Disabling waves does not bypass live limits. A lead wave that cannot fit its configured domain ceiling is rejected before actors spawn, with a configuration error. Other operations can also consume the available slots; existing spawn acknowledgement timeouts remain the bound while waiting for capacity.

## Tactical behavior

### Evidence and search

Assigned defenders and operation-owned units already publish validated hostile-damage snapshots to the role-scoped report registry. The director checks for fresh reports within the defended town radius plus its configured search margin. It stores positions, not an enemy entity or a continuously updated player position.

Fresh ground reports can retask at most every 15 seconds. A first contact, expiry or cumulative movement beyond 75 metres from the last commanded contact changes the search version. Small incremental reports cannot prevent eventual retasking once their cumulative displacement exceeds that threshold. Stale/missing contact returns search to the town.

Infantry first investigates its contact sector, moves through four vicinity sectors, then holds a sector. A leg advances on arrival or its bounded timeout. IFV-delivered infantry begins on a flank sector while motorized/foot troops investigate contact. Points remain inside the defended vicinity. New waves and lost packages redistribute the surviving packages across available sectors, subject to the same retask interval.

This assigns investigation and containment orders to native AI. It does not implement room-by-room building clearance, omniscient detection, coordinated suppression or guaranteed road encirclement. Native navigation/perception/combat still determine how units move and fight.

### Ground delivery and support

Launch offsets run laterally around the selected origin. Separate approach wedges are projected onto reachable road points; an unavailable road result rejects that package rather than guessing an API or teleporting vehicles.

Passenger vehicles use their profile dismount distance, increased to at least 200 m when a fresh local hostile report exists. A vehicle must reach its approach area and slow below the configured code threshold before normal passenger exits begin. Infantry tasks start only after the living passengers are actually out. A stalled ingress has a 120-second leg deadline and orders withdrawal instead of waiting forever.

The truck waits at the drop point. An IFV or armour package receives its own bounded reachable road cover point, normally on the origin side of reported contact. Infantry and vehicle task timers are independent, so passenger searching cannot starve vehicle support orders. Cover is reconsidered every 45 seconds. This is not a convoy driving controller and does not guarantee matching arrival times or terrain line of sight.

Player-commanded groups are not retasked. Existing faction-change and vehicle-destruction validation remains in the native monitor.

### Air insertion and gunships

The existing incremental LZ planner, separate pads, direct landing, verified touchdown, animated exits, departure and predictive traffic yielding remain in use. Later-wave aircraft keep their boarding hold until they receive flight orders, even though the parent operation is already ACTIVE.

Landed air infantry uses the same rate-limited tactical search/sector logic as ground infantry. Aircraft departures are not counted as infantry casualties. Gunships continue to investigate recent allied reports and vary passes through the existing engagement and traffic policies; firing still requires native perceived hostile targets and friendly-clearance checks. The tactical commander adds wave admission, task duration and withdrawal, rather than replacing REAPER flight control. Collision-free flight and FPS are not certified by these source changes.

Ground recipes still cannot contain air profiles. Ground combined arms and complete air packages share the tactical policy, but a truck + helicopter + gunship recipe under one operation is a remaining executor extension.

## Withdrawal and persistence

The whole QRF withdraws after 600 active seconds, when target control is lost, when its recorded outpost is lost, or when known living strength reaches 40% of readied starting strength and no reserve remains available. An individual package can withdraw at 50% strength. Ground strength includes its soldiers/crew; air tactical strength counts the inserted infantry so routine aircraft departures do not look like casualties.

Vehicles receive reachable return orders and dismounted troops move toward the safe origin. There is no teleport/remount shortcut. If an origin outpost changes hands, the commander seeks another eligible same-role outpost. If none exists, it holds troops/vehicles rather than ordering them back to the captured base. A missing return road also produces a hold. Configured fallback origins have no ownership entity to monitor.

Ground terminal retention requires **both** at least 120 seconds of age and at least 1,200 m separation between every owned member/vehicle and every controlled player. Stationary grounded passenger survivors may exit normally. Nearby wrecks and survivors remain, with their aggregate capacity reservation. Retained living packages also occupy the live-ground limit; corpse-only packages do not occupy that live limit but still hold aggregate budget until cleanup. Air retains its existing separate retention policy (600 seconds and 1,500 m by the current fixture). World unload releases the session records.

This is session retention, not saved campaign persistence. A force is not declared victorious, ownership is not changed and resources are not refunded by tactical arrival or withdrawal.

## Settings and performance

All settings are attributes on the existing operation executor. Tactical coordination defaults OFF for other scenarios and is explicitly ON in Chicken.

| Attribute | Chicken value | Purpose |
|---|---:|---|
| `m_bQRFTacticalCommander` | 1 | Shared ground/air tactical policy |
| `m_bQRFTwoWaves` | 1 | Keep the reserve unspawned |
| `m_iQRFSplitTier` | 3 | Minimum tier; also requires at least four elements |
| `m_iQRFReinforcementDelay` | 60 s | Normal reserve release |
| `m_iQRFReserveWait` | 300 s | Finite reserve deadline after first dispatch |
| `m_iQRFMaxLiveGround` | 6 | Live ground packages across the executor |
| `m_iQRFMaxLiveAircraft` | 6 | Live transport/gunship airframes across the executor |
| `m_iQRFTacticalDuration` | 600 s | Active task deadline |
| `m_iQRFOrderInterval` | 15 s | Contact/roster/infantry retask interval |
| `m_fQRFThreatDismount` | 200 m | Reported-threat dismount standoff |
| `m_fQRFWithdrawFraction` | 0.4 | Whole-force strength threshold without reserve |
| `m_fQRFLocalWithdrawFraction` | 0.5 | Local package strength threshold |
| `m_iQRFGroundRetention` | 120 s | Minimum ground retention age |
| `m_fQRFGroundCleanupDistance` | 1,200 m | Ground all-player cleanup distance |

The director evaluates each operation every five seconds on the existing server timer. Construction is limited to one ground element or air pair per tick; no per-frame search loop is added. Routine orders and road lookups are throttled. Full allocations, including virtual reserves and retained forces, still count against the original aggregate group/personnel/vehicle budgets.

The six-package ceiling does **not** mean six soldiers. Each package can include infantry and crew, and multiple retained forces may continue to use aggregate budget. Waves reduce simultaneous construction and constrain live deployment; they do not guarantee a particular server FPS. Measure spawn-time spikes, server frame time and AI counts before increasing limits. Retention intentionally trades available capacity for persistence near players.

## Verification and next Play test

Offline checks exercise source-adapted policy/lifecycle bodies with mocked engine operations: 582 policy assertions across five tiers and eight sizes, 25 ground lifecycle assertions, 29 air lifecycle assertions, 11 contact/roster assertions and three retained-aircraft counter cases passed. Installed-resource/configuration checks also passed for 25 GUIDs, navigation, eight profiles, all prepared levels/recipes, tactical settings and the disabled Invader gate. Workbench smoke tests are connected to the existing optional test runner and do not create QRFs or enable Invaders. These checks cannot establish Enforce compilation, native navigation, boarding, flight or multiplayer behavior.

1. Stop Play, compile Game scripts successfully, then reload `AROR_Everon_Chicken` from disk.
2. Run the existing US defender test in Provins. Expect `[AROR TACTICAL]` with tier 1, first wave 2, reserve 0, followed by one truck and one LAV under one ACTIVE operation. Both must remain `AROR_OCCUPIERS`.
3. Let them arrive. Verify passenger exit before infantry search, distinct search sectors, truck holding and IFV road cover. Attack another assigned defender away from the first contact to check bounded retasking. Then stop firing and check return to town search after report expiry.
4. Test active package losses, nearby survivor/wreck retention, withdrawal and terminal cancellation. Check capacity stays held near players and releases only after age AND all-player distance. Test with a second player near the force as well.
5. In a separate explicit test configuration, use a tier-4/5 ground response with at least four elements. Verify first-wave size, 60-second reserve delay, live cap, finite unused-reserve expiry and no duplicate ACTIVE acknowledgement. The normal truck/LAV fixture has no second wave by design.
6. For flight testing, clear the ground override, explicitly select the existing REAPER test and level 5. Expect three pairs initially and at most six live aircraft; later pairs need two free slots. Check boarding holds, independent pads, actual exits, gunship reports, traffic yielding and departure before judging performance.
7. Keep Invader admission OFF during these tests. Enable a separate USSR test only when explicitly ready to establish valid Invader-owned targets/origins. The optional readiness preview remains disconnected from dispatch and stock spending.

Extenders: opt in on the executor, preserve immutable tier/package semantics and account for virtual/retained capacity. Existing non-tactical scenarios retain their execution path. Resource owners can later supply readiness/reservation data through the existing provider seam; no factory, garrison or territory implementation is duplicated here.
