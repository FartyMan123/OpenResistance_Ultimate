# Add MARS, territory attacks and campaign UI updates

## Summary

Add a role-separated MARS response debugger and delayed Occupier counterattacks, sharing QRF force selection and execution. Adopt Readiness and Crackdown throughout the campaign and rebuild REBINTEL, notifications, map markers and setup around the UI design system. Introduce the port-gated Black Market foundation.

## Changes

- **MARS / response scaling:** red landscape app with a response-type grid, separate US/USSR catalogues, simulated Readiness and Crackdown sliders, and live force previews. A shared modular policy resolves equipment and intensity without changing campaign metrics or balances. Requests target the nearest Resistance-owned territory and require a safe military launch origin owned by the selected army. Explicit airborne responses use REAPER; ground selections retain ground execution.
- **Territory attacks:** optional Occupier attacks on Resistance-held military sites require a capture QRF to activate and finish, at least 15 minutes since capture, and five minutes after that QRF ends. One attack is outstanding globally, with a 15-minute rest after completion. Attacks reuse QRF funding, actor budgets, tactical coordination and cleanup; normal capture authority determines success. Debug responses do not unlock attacks.
- **QRF execution:** improve origin selection with distance, water-crossing cost, contention, garrison depletion and launch cooldown checks. Add attack rally/release coordination, defender reporting and town pacification. Address Lamentin failures with paused initial boarding, guarded stale-boarding recovery, a wider Chicken road search and bounded airborne landing-site fallback/local adjustment. Landing safety checks remain enforced; successful native Lamentin deployment is unverified.
- **Flight and route feedback:** the later North Ridge run delivered three airborne packages and recaptured the target. Gunship support now commands lower run-in/crossing passes, with traffic-yield altitude capped at cruise height. Ground recovery detects continued movement without net objective progress and can try a bounded road/foot approach. Native flight, engagement and route improvements need a fresh Play test.
- **Campaign terminology:** rename War Level/Aggression to Readiness/Crackdown across methods, attributes, presets, tests and documentation. Add army-specific threat labels. Operation serializer version 3 retains loading support for versions 1 and 2.
- **REBINTEL / notifications:** army tabs, labelled level cards, configurable per-player HUD with army flags, level-change alerts, redesigned headline banners and card toasts, shared colour tokens and explicit Roboto Condensed fonts.
- **Headline clipping:** include the main title in banner sizing, constrain width to the viewport, and wrap each row with reserved height. Cached geometry avoids repeated text layout work while the timer animates. Retest long territory names and Cyrillic labels at different UI scales.
- **Phone / map / setup:** rounded home tiles, military-frame location symbols, location names, green HICOMM roads and the base-game map by default. A full-screen start/load menu replaces the initial dialogs; faction and HQ steps retain configurable dialogs.
- **Black Market / locations:** add a landscape phone catalogue gated by owned seaports, server-validated personal-money orders and owner-only order replies. Add three Chicken seaports and rename 21 sites without changing stable IDs. Orders are session-only; delivery and persistence remain unfinished.
- **Documentation:** update architecture, UI guidance, campaign locations, operation/force-pool references and MARS acceptance steps.

## Testing

- Passed 1,219 focused mocked assertions: MARS/attacks/response matrix (1,011), landing search (159), and boarding recovery (49).
- Also passed Invader campaign (44 assertions) and Everon QRF integration (50 assertions).
- North Ridge follow-up checks passed 85 support/banner/detour assertions, 45 ingress assertions, 40 unloading assertions, and the existing 49 boarding and 159 landing-search assertions. MARS/attacks remained at 1,011 passing assertions. These use mocked native services.
- Installed-asset/configuration validation passed 27 base-game GUIDs, 32 recipes and all 50 faction/Readiness/Crackdown budget combinations, including updated Chicken launch settings. REAPER pilot and flight-waypoint references were previously checked against the installed REAPER database.
- `git diff --check` passed.
- Broader checks currently fail: `Test-RebIntel.js` on direct menu/app layout binding coverage; `Test-QRFProgression.js` on the phone response-intensity wording assertion; `Test-QRFStrategy.js` on a missing `AROR_FactionRoles` mock; and `Test-QRFTownHold.js` on an unresolved `IsBroken` helper in its harness. These suites are not recorded as passing.
- **Latest combined changes have not been compiled or played in Workbench.** Compile in Script Editor, reload Chicken, then check phone/layout loading, both factions and all response types, Lamentin road deployment/boarding/flight, dedicated-server requests, old-save migration, and attack timing/capture outcomes. Mocked checks cannot establish native driving, flight or replication correctness.

## Notes

- **Extender changes:** update renamed Readiness/Crackdown APIs and attributes, including `GetWarLevel`, `GetAggression*`, `SetWarLevel_S`, `m_iMinimumWarLevel`, `m_iMaximumWarLevel` and `m_iStartingAggression`. The resource seam is now `AROR_QRFAvailabilityProvider` / `AROR_QRFAvailabilitySnapshot`, `CaptureAvailability_S` and `RegisterQRFAvailabilityProvider_S`. Recipes expose `m_eResponseType`; response families can extend the shared selector and catalogue.
- The server owns dispatch, orders, funding, capture and attack admission. New player-controller RPCs support MARS and Black Market requests; the latest MARS debug status is replicated. Matching server/client scripts and layouts are required. REBINTEL remains read-only, with a client HUD timer.
- Live operations, attack eligibility/cooldowns and Black Market orders are session-only. Loaded holdings require a capture QRF in the current session before automatic attacks become eligible. Black Market payment/order recording exists before delivery; treat it as an unfinished feature.
- New MARS, Black Market and setup layout metadata GUIDs were hand-authored and need Workbench registration/verification. The existing experimental REAPER dependency remains required for airborne execution.
