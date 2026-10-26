"""Issue-only Workshop dependency monitoring; Python standard library only."""
import argparse
import json
import os
from pathlib import Path
import re
import sys
import urllib.request

PREFIX = "Dependency-watch metadata: "
LABELS = {"dependency-watch": "5319e7", "compatibility:pending": "fbca04", "compatibility:passed": "0e8a16", "compatibility:failed": "b60205"}

def api(url, token=None, method="GET", data=None):
    headers = {"User-Agent": "reforger-dependency-watch/1.0", "Accept": "application/json"}
    if token:
        headers["Authorization"] = "Bearer " + token
    if data is not None:
        headers["Content-Type"] = "application/json"
    req = urllib.request.Request(url, headers=headers, method=method,
        data=None if data is None else json.dumps(data).encode())
    with urllib.request.urlopen(req, timeout=30) as response:
        if response.status == 202 or response.headers.get("X-Cache", "").upper() == "STALE":
            raise RuntimeError("Metadata pending or stale; no decision made")
        return json.load(response)

def meta(issue):
    for line in (issue.get("body") or "").splitlines():
        if line.startswith(PREFIX):
            try:
                value = json.loads(line[len(PREFIX):])
                if isinstance(value, dict) and value.get("schema") == 1:
                    return value
            except ValueError:
                pass
    return {}

def tracked(issues, mod_id, version):
    return any(meta(i).get("modId") == mod_id and meta(i).get("version") == version for i in issues)

def baseline(issues, dep):
    results = []
    for issue in issues:
        m = meta(issue)
        labels = {x["name"] for x in issue.get("labels", [])}
        if (m.get("modId") == dep["modId"] and m.get("version")
            and issue.get("state") == "closed" and "compatibility:passed" in labels
            and not labels.intersection({"compatibility:pending", "compatibility:failed"})):
            results.append((issue.get("closed_at") or "", issue["number"], m["version"]))
    return max(results)[2] if results else dep.get("initialWorkingVersion")

def version_of(payload, mod_id):
    mod = payload.get("mod", {})
    if payload.get("status") != "success" or str(mod.get("id", "")).upper() != mod_id:
        raise ValueError("Unexpected API response or mod ID")
    version = mod.get("version")
    if not isinstance(version, str) or not version.strip() or len(version) > 100:
        raise ValueError("Missing/invalid version")
    return version

def body_for(dep, version, previous, payload):
    lines = [
        "Manual compatibility retest required. Detection is not a test result.",
        "", "Dependency: " + dep["name"], "Workshop: " + dep["workshopUrl"],
        "Version requiring testing: " + version,
        "Last confirmed working: " + (previous or "None recorded"),
        "Reported game version: " + str(payload["mod"].get("gameVersion", "Unknown")),
        "Metadata updated: " + str(payload["mod"].get("updatedAt", "Unknown")),
        "Source: unofficial ReforgerMods API (cached metadata).", "",
        "## Test environment (fill in manually)", "- Our mod commit:",
        "- Actual Reforger version:", "- Actual installed dependency version:",
        "- Other dependency versions:", "", "## Checklist",
        "- [ ] World loads without script errors", "- [ ] Flagpole capture works",
        "- [ ] Expected AI/helicopters spawn and behave correctly",
        "- [ ] Despawning and cleanup work",
        "- [ ] Repeated cycles do not create duplicates",
        "- [ ] Multiplayer/server behaviour checked where applicable", "",
        "## Result and evidence", "Record Passed / Failed / Not tested and attach notes/logs.",
        "Remove compatibility:pending, apply exactly one result label",
        "(compatibility:passed or compatibility:failed), then close the issue.",
        "Closing alone never means passed. Do not mark superseded tests passed.", "",
        "## Tracking metadata (leave unchanged)",
        PREFIX + json.dumps({"schema": 1, "modId": dep["modId"], "version": version})]
    return "\n".join(lines)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--config", default=str(Path(__file__).resolve().with_name("dependency-watch.json")))
    args = parser.parse_args()
    repo = os.environ.get("GITHUB_REPOSITORY", "")
    token = os.environ.get("GH_TOKEN", "")
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repo) or not token:
        raise ValueError("Set GITHUB_REPOSITORY=owner/repo and GH_TOKEN")
    with open(args.config, encoding="utf-8") as f:
        deps = json.load(f)["dependencies"]
    seen = set()
    for dep in deps:
        dep["modId"] = dep["modId"].upper()
        if not re.fullmatch(r"[A-F0-9]{16}", dep["modId"]) or dep["modId"] in seen:
            raise ValueError("Invalid or duplicate mod ID")
        seen.add(dep["modId"])
    base = "https://api.github.com/repos/" + repo
    issues, page = [], 1
    while True:
        batch = api(base + f"/issues?state=all&per_page=100&page={page}", token)
        issues.extend(i for i in batch if "pull_request" not in i)
        if len(batch) < 100:
            break
        page += 1
    labels_ready = False
    failures = []
    for dep in deps:
        try:
            payload = api("https://api.reforgermods.net/v2/mods/" + dep["modId"])
            version = version_of(payload, dep["modId"])
            previous = baseline(issues, dep)
            if tracked(issues, dep["modId"], version) or version == previous:
                print(f"{dep['name']} {version}: already tracked or confirmed")
                continue
            title = f"Compatibility retest: {dep['name']} v{version}"
            body = body_for(dep, version, previous, payload)
            if args.dry_run:
                print("DRY RUN: " + title + "\n" + body)
                continue
            if not labels_ready:
                existing, p = set(), 1
                while True:
                    batch = api(base + f"/labels?per_page=100&page={p}", token)
                    existing.update(x["name"] for x in batch)
                    if len(batch) < 100:
                        break
                    p += 1
                for name, color in LABELS.items():
                    if name not in existing:
                        api(base + "/labels", token, "POST", {"name": name, "color": color})
                labels_ready = True
            issue = api(base + "/issues", token, "POST", {"title": title, "body": body,
                "labels": ["dependency-watch", "compatibility:pending"]})
            issues.append(issue)
            print("Created " + issue["html_url"])
        except Exception as exc:
            failures.append(dep["name"] + ": " + str(exc))
    if failures:
        raise RuntimeError("\n".join(failures))

if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print("Dependency check failed: " + str(exc), file=sys.stderr)
        sys.exit(1)
