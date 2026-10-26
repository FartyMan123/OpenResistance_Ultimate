"""Create GitHub compatibility-test issues for new Reforger Steam builds."""
import argparse, json, os, re, sys, urllib.request
from pathlib import Path

PREFIX = "Reforger-release-watch metadata: "
LABELS = {
    "reforger-release-watch": "1d76db", "release:production": "0e8a16",
    "release:experimental": "bf8700", "compatibility:pending": "fbca04",
    "compatibility:passed": "0e8a16", "compatibility:failed": "b60205",
}

def api(url, token=None, method="GET", data=None):
    headers = {"User-Agent": "reforger-release-watch/1.0", "Accept": "application/vnd.github+json", "X-GitHub-Api-Version": "2022-11-28"}
    if token: headers["Authorization"] = "Bearer " + token
    if data is not None: headers["Content-Type"] = "application/json"
    req = urllib.request.Request(url, headers=headers, method=method, data=None if data is None else json.dumps(data).encode())
    with urllib.request.urlopen(req, timeout=30) as response:
        return json.load(response)

def meta(issue):
    for line in (issue.get("body") or "").splitlines():
        if line.startswith(PREFIX):
            try:
                value = json.loads(line[len(PREFIX):])
                return value if isinstance(value, dict) and value.get("schema") == 1 else {}
            except ValueError: pass
    return {}

def labels_of(issue):
    return {x["name"] if isinstance(x, dict) else str(x) for x in issue.get("labels", [])}

def tracked(issues, channel, build):
    return any(meta(i).get("channel") == channel and str(meta(i).get("buildId")) == str(build) for i in issues)

def baseline(issues, channel):
    found = []
    for issue in issues:
        m, labels = meta(issue), labels_of(issue)
        if (m.get("channel") == channel["id"] and m.get("buildId") and issue.get("state") == "closed"
                and "compatibility:passed" in labels
                and not labels.intersection({"compatibility:pending", "compatibility:failed"})):
            found.append((issue.get("closed_at") or "", issue.get("number", 0), str(m["buildId"])))
    return max(found)[2] if found else channel.get("initialWorkingBuildId")

def parse_public_build_id(text):
    stack, pending = [], None
    pair = re.compile(r'^\s*"([^"]+)"\s+"([^"]*)"\s*$')
    key = re.compile(r'^\s*"([^"]+)"\s*$')
    for raw in text.splitlines():
        line = raw.strip()
        match = pair.match(line)
        if match:
            name, value = match.groups()
            if stack[-2:] == ["branches", "public"] and name == "buildid":
                if not re.fullmatch(r"[1-9][0-9]*", value): raise ValueError("Invalid public build ID")
                return value
            pending = None; continue
        match = key.match(line)
        if match: pending = match.group(1); continue
        if line == "{":
            if pending is not None: stack.append(pending); pending = None
        elif line == "}":
            if stack: stack.pop()
            pending = None
    raise ValueError("Could not find branches.public.buildid in SteamCMD output")

def body_for(channel, build, previous):
    metadata = {"schema": 1, "channel": channel["id"], "steamAppId": channel["steamAppId"], "buildId": str(build)}
    return "\n".join([
        "Manual Reforger compatibility testing is required. Detection is not a test result.", "",
        "Channel: " + channel["name"], "Steam App ID: " + channel["steamAppId"],
        "Steam public build ID requiring testing: " + str(build),
        "Last confirmed working build ID: " + (str(previous) if previous else "None recorded"),
        "Detection source: SteamCMD app_info_print, public branch.", "",
        "## Test environment (fill in manually)", "- Actual Arma Reforger version shown in-game:",
        "- Our mod commit/release:", "- Dedicated server build ID/version:", "- Dependency versions:", "- Test platform:", "",
        "## Checklist", "- [ ] Client starts and reaches the main menu", "- [ ] Dedicated server starts and accepts a connection",
        "- [ ] Mod downloads and loads without script/resource errors", "- [ ] Test world/scenario loads",
        "- [ ] Flagpole and capture behaviour works", "- [ ] Expected AI and helicopter spawning works",
        "- [ ] Despawning and cleanup work", "- [ ] Repeated cycles do not create duplicate entities",
        "- [ ] Multiplayer replication/JIP behaviour checked", "- [ ] Relevant server and client logs attached or linked", "",
        "## Result and evidence", "Record Passed, Failed, or Not tested and include notes/logs.",
        "Remove compatibility:pending, apply exactly one of compatibility:passed or compatibility:failed, then close the issue.",
        "Closing alone is not a pass. Do not mark an untested or superseded build as passed.", "",
        "## Tracking metadata (leave unchanged)", PREFIX + json.dumps(metadata, separators=(",", ":"))])

def validate_config(config):
    channels = config.get("channels")
    if not isinstance(channels, list) or not channels: raise ValueError("Config must contain channels")
    ids, apps = set(), set()
    for c in channels:
        if any(not c.get(k) for k in ("id", "name", "steamAppId")): raise ValueError("Each channel requires id, name and steamAppId")
        c["steamAppId"] = str(c["steamAppId"])
        if not re.fullmatch(r"[a-z0-9-]+", c["id"]) or not re.fullmatch(r"[1-9][0-9]*", c["steamAppId"]): raise ValueError("Invalid channel")
        initial = c.get("initialWorkingBuildId")
        if initial is not None and not re.fullmatch(r"[1-9][0-9]*", str(initial)): raise ValueError("Invalid initial build")
        if c["id"] in ids or c["steamAppId"] in apps: raise ValueError("Duplicate channel or App ID")
        ids.add(c["id"]); apps.add(c["steamAppId"])
    return channels

def paged(base, path, token):
    results, page = [], 1
    while True:
        batch = api(base + path + ("&" if "?" in path else "?") + f"per_page=100&page={page}", token)
        results.extend(batch)
        if len(batch) < 100: return results
        page += 1

def ensure_labels(base, token):
    existing = {x["name"] for x in paged(base, "/labels", token)}
    for name, color in LABELS.items():
        if name not in existing: api(base + "/labels", token, "POST", {"name": name, "color": color})

def main(argv=None):
    here = Path(__file__).resolve().parent
    p = argparse.ArgumentParser(description=__doc__); p.add_argument("--dry-run", action="store_true")
    p.add_argument("--config", default=str(here / "reforger-releases.json")); p.add_argument("--appinfo-dir", default=str(here / "appinfo")); args = p.parse_args(argv)
    repo, token = os.environ.get("GITHUB_REPOSITORY", ""), os.environ.get("GH_TOKEN", "")
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repo) or not token: raise ValueError("Set GITHUB_REPOSITORY=owner/repo and GH_TOKEN")
    channels = validate_config(json.loads(Path(args.config).read_text(encoding="utf-8")))
    base = "https://api.github.com/repos/" + repo
    issues = [x for x in paged(base, "/issues?state=all", token) if "pull_request" not in x]
    ready, failures = False, []
    for channel in channels:
        try:
            build = parse_public_build_id((Path(args.appinfo_dir) / (channel["steamAppId"] + ".txt")).read_text(encoding="utf-8", errors="replace"))
            previous = baseline(issues, channel)
            if tracked(issues, channel["id"], build) or str(build) == str(previous):
                print(f"{channel['name']} build {build}: already tracked or confirmed"); continue
            title, body = f"Reforger {channel['name']} compatibility test: Steam build {build}", body_for(channel, build, previous)
            if args.dry_run: print("DRY RUN: " + title + "\n" + body); continue
            if not ready: ensure_labels(base, token); ready = True
            issue = api(base + "/issues", token, "POST", {"title": title, "body": body, "labels": ["reforger-release-watch", "release:" + channel["id"], "compatibility:pending"]})
            issues.append(issue); print("Created " + issue["html_url"])
        except Exception as exc: failures.append(channel.get("name", "unknown") + ": " + str(exc))
    if failures: raise RuntimeError("\n".join(failures))

if __name__ == "__main__":
    try: main()
    except Exception as exc: print("Reforger release check failed: " + str(exc), file=sys.stderr); sys.exit(1)
