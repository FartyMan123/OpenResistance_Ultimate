# Addon dependency compatibility watch

All supporting files live in tools/tests/addon-dependencies/.
The workflow remains in .github/workflows/dependency-watch.yml because GitHub
requires workflows in that directory. Its run steps use this folder as their
working directory. The checker resolves its default JSON config beside itself,
so it also works when invoked from the repository root.

## Install or upgrade
Extract into your repository root and commit to the default branch.
Enable Issues and Actions. The workflow needs contents:read and issues:write.
No personal access token, pip packages, game installation or self-hosted runner.
When upgrading from the previous ZIP, remove the old copies:
- tools/check_dependencies.py
- tools/tests/test_check_dependencies.py
- dependency-watch.json (repository root)
- DEPENDENCY-WATCH.md (repository root)
Replace .github/workflows/dependency-watch.yml with this updated version.
If you edited your old dependency config, transfer those entries/baselines into
this folder's dependency-watch.json before removing the old copy.

Optionally set initialWorkingVersion in the local JSON to a version actually
tested. It is null because no known-working version was supplied.
Actions > Dependency compatibility watch > Run workflow: enable dry_run to preview,
then run without dry_run to create the issue. Scheduled daily at 02:23 UTC.
No PR/push triggers, automated PRs, commits, branch gates or game launches.
First normal run requests a baseline retest if no known-working version exists.

## Manual results
Fill in environment/checklist and add notes/logs. Remove compatibility:pending,
apply exactly one of compatibility:passed or compatibility:failed, then close.
Closing alone is not passing. Do not mark superseded/untested issues passed.
The latest closed passed issue by close timestamp supplies the baseline; otherwise
initialWorkingVersion is used. This is not semantic version ordering.
Open and closed issues prevent duplicates for the same mod/version. Leave visible
tracking metadata unchanged. Reopen an existing issue to retest the same version.
New versions create separate issues; old issues are not auto-closed.

## Limitations
Uses unofficial cached ReforgerMods API v2; detection may lag Workshop.
Only the current version is checked. Intermediate releases and same-version
republishing may be missed. Version strings are compared for equality.
Explicitly stale/pending responses fail the run; unmarked cache can still lag.
API/schema/permission failures fail Actions, not PRs. Check Actions for failures.
No automatic retries; the next scheduled/manual run tries again.
Workflow concurrency prevents its own runs overlapping; do not run another writer
concurrently because issue check/create is not atomic.
Issues are the state store. Deleting/editing metadata loses tracking state.
Label/body editors can influence the record; this is not a security gate.
Results apply to recorded environments. Your own mod/game updates do not
invalidate historical passes automatically.
No live API/GitHub integration verified. Offline tests use synthetic fixtures.

## Commands from repository root
python -m unittest discover -s tools/tests/addon-dependencies/tests -v

For a local API preview, set GH_TOKEN and GITHUB_REPOSITORY=owner/repo, then:
python tools/tests/addon-dependencies/check_dependencies.py --dry-run

## References
- [API documentation](https://reforgermods.net/arma-reforger-mods-api/v2/)
- [GitHub workflow location](https://docs.github.com/en/actions/concepts/workflows-and-actions/workflows)
