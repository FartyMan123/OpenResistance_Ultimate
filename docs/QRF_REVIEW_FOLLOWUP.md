# QRF review follow-up

Reviewed the six reported issues against `f6277ac`. The changes below need a fresh Workbench compile and native acceptance. Current automated evidence: 28 suites, 97,342 mocked assertions and 35 detected mutations. See [release validation](QRF_RELEASE_VALIDATION.md) for source hashes and broader limits.

| Issue | Finding and action | Remaining limit |
|---|---|---|
| One dispatch failure ends the operation | Confirmed for active reinforcement. `TryActivateOperation` now calls `FailPackage`. First/middle/last failure tests preserve healthy packages and prevent duplicate orders during nested activation. | Required composite lead loss during SPAWNING intentionally still aborts preparation. Native orders already issued cannot be atomically undone; terminal retention applies. |
| Boarding timeout | Initial preparation already had a manager deadline: default 30 seconds, Chicken 90 seconds, or the larger finite landing-search allowance (121 seconds with current candidates). Active reserves lacked their own short preparation limit. | New `m_iReaperPreparationTimeout` defaults to 120 seconds, validated at 30–600. Construction freezes the deadline; dormant groups, seat requests and the ready barrier cannot reset it. The initial global deadline may expire earlier. Physical actors retain capacity/collateral until safe cleanup. |
| Last `GetInVehicle` argument | Installed Workbench `CompartmentAccessComponent` API declares the last parameter as `bool performWhenPaused`. Added an inline explanation for collision-only initial boarding and continued actual occupancy checks. | Native paused boarding remains to be tested. |
| Settings bounds | Both actual-source validators pass their attribute defaults and every serialized executor in shipped `.et`, `.ent` and `.layer` resources. Only Chicken `Managers.layer` contains an executor instance. Exact tested values are in the interruption suite's automated output. | Extender overrides outside the repository need their own validation. No bounds were silently loosened. |
| Headline sizing and missing helper | Confirmed inconsistent factors. Both width and row estimates now use one UTF-8-aware 0.75 helper. `AROR_TextUtils.CharCount` exists in `Scripts/Game/Common`; actual-source tests cover signed/unsigned UTF-8 bytes, Cyrillic and supplementary characters. | **Visual check NOT_RUN.** This remains a conservative estimate, not proportional-font measurement. The previous `MeasureText` helper reads laid-out widget size, which becomes the assigned frame after explicit placement. Tests cannot establish glyph widths or rendered wrapping. |
| Null asset classified recovered | Cleanup now logs an **unobserved removal** and releases its collateral without incrementing recovered value. Confirmed deaths still debit Economy; a missing reference alone cannot establish death. Tests distinguish both cases and preserve later-wave collateral. | If an external deletion erases a destroyed asset before any observation, the bill cannot be reconstructed from null. Owned cleanup observes losses before deletion. Native/external deletion ordering remains a risk to verify. |

Four added mutations prove the tests reject operation-wide active dispatch failure, missing reserve preparation deadlines, recovered classification for unknown removals and divergent banner estimates.

## Shipped settings

Both validators run against all validated fields, not only the selected values below. Defaults and Chicken pass unchanged.

| Setting | Default / Chicken | Valid range |
|---|---:|---:|
| Attack staging distance | 500 / 500 m | 200–1500 |
| Attack staging timeout | 180 / 180 s | 30–600 |
| Origin cooldown | 600 / 600 s | 0–3600 |
| Preferred minimum origin distance | 600 / 600 m | 0–3000 |
| Origin water penalty | 2 / 2 | 1–10 |
| Maximum origin garrison loss | 0.5 / 0.5 | 0–1 |
| Town support per minute | 6 / 6 | 0–30 |
| Reserve wait | 300 / 450 s | 60–480; greater than reinforcement delay |
| Road launch search radius | 150 / 300 m | 75–300 |
| Air package preparation timeout | 120 / 120 s | 30–600 |

## Native checks

1. Compile current source, reload Chicken and test both armies. Block pilot/passenger boarding during an ACTIVE reserve. It must time out at its frozen construction deadline, degrade only that package and use safe retention. Check initial boarding against the global timeout separately.
2. Invalidate a ready ACTIVE reserve's crew or waypoint immediately before dispatch. Healthy earlier/later packages must receive orders once. During SPAWNING, losing a required lead element must abort preparation without a false ACTIVE event.
3. Repeat paused initial pilot boarding, verifying the pilot actually occupies the correct compartment before flight.
4. Exercise long title/source/detail strings, including `ТЕРРИТОРИЯ ПОД АТАКОЙ / СЕВЕРНЫЙ ХРЕБЕТ` and `ГОТОВНОСТЬ / СОВЕТСКАЯ АРМИЯ`, at multiple resolutions and UI scales. Inspect clipping, early wrapping, unused height and the timer. No visual success is claimed by this audit.
5. Compare a confirmed tracked destruction with an external deletion before observation. The former must charge once; the latter must log unknown removal and never claim recovery or fabricate income. Inspect reserved amounts for surviving assets and future waves.

Existing [native acceptance steps](QRF_LIGHT_RESPONSE_ACCEPTANCE.md) still apply. The review introduces no save-format or replication change. Extenders may configure `m_iReaperPreparationTimeout` within its validated range; required-lead and active-package failure semantics are documented above.
