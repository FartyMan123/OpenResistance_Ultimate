# Arma Reforger release compatibility watch

Monitors the Steam public build IDs for Production (`1874880`) and Experimental (`1890860`). A newly observed channel/build pair creates a GitHub issue with `compatibility:pending`.

## Install and run

Copy this package into the repository root and commit it to the default branch. Enable GitHub Issues and Actions. Run **Actions > Reforger release compatibility watch > Run workflow** with `dry_run` enabled first, then run normally to create issues.

Both initial baselines are `null`, so the first normal run creates one pending issue per channel. To suppress that, put a known-working numeric Steam build ID in `initialWorkingBuildId` before the first run.

## Recording results

1. Fill in the environment, checklist and evidence.
2. Remove `compatibility:pending`.
3. Apply exactly one of `compatibility:passed` or `compatibility:failed`.
4. Close the issue.

Closing alone is not passing. Production and Experimental have independent baselines. Open and closed issues prevent duplicates for the same channel/build. Reopen an issue to retest the same build. Do not edit the tracking metadata.

## Detection

The workflow downloads SteamCMD from Valve, captures `app_info_print`, and parses `branches.public.buildid`. The Steam build ID is the automatic release identity. Enter the in-game Reforger version manually during testing.

## Local tests

```bash
python -m unittest discover -s tools/tests/reforger-releases/tests -v
```

For a local dry run, save SteamCMD outputs as `appinfo/1874880.txt` and `appinfo/1890860.txt`, set `GH_TOKEN` and `GITHUB_REPOSITORY=owner/repo`, then run `python check_releases.py --dry-run`.

## Limitations

It detects public Steam build changes but does not install or launch Reforger. Same-build republishing cannot be detected. SteamCMD, network, GitHub API, permission or schema failures fail the Action. GitHub Issues are the state store, not a security boundary.
