"""Copy historical releases without changing tags or published asset bytes.

This manually invoked migration uses the destination workflow's short-lived
GITHUB_TOKEN. It never needs the account's personal access token.
"""
from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request

SOURCE = "DashSaman/MyTelSoftPhone"
TARGET = "mytel-one/MytelPhone"
API = "https://api.github.com/"
TOKEN = os.environ.get("GITHUB_TOKEN", "")
HEADERS = {"Authorization": "Bearer " + TOKEN, "Accept": "application/vnd.github+json",
           "X-GitHub-Api-Version": "2022-11-28", "User-Agent": "MyTel-exact-release-migration"}


def api(path, method="GET", payload=None):
    data = None if payload is None else json.dumps(payload).encode()
    request = urllib.request.Request(API + path, method=method, data=data, headers=HEADERS)
    if data is not None:
        request.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(request, timeout=90) as response:
            return None if response.status == 204 else json.load(response)
    except urllib.error.HTTPError as error:
        raise RuntimeError(f"GitHub {method} {path.split('?')[0]} returned HTTP {error.code}") from None


def pages(path):
    result = []
    for page in range(1, 101):
        rows = api(path + ("&" if "?" in path else "?") + f"per_page=100&page={page}")
        if not isinstance(rows, list):
            raise RuntimeError("Expected a paginated list")
        result.extend(rows)
        if len(rows) < 100:
            return result
    raise RuntimeError("Unexpected pagination bound")


def asset_identity(asset):
    name = asset["name"]
    if not name or PurePosixPath(name).name != name or any(c in name for c in "\\\r\n\x00"):
        raise RuntimeError("Unsafe asset basename")
    digest = asset.get("digest", "")
    if not re.fullmatch(r"sha256:[0-9a-f]{64}", digest or ""):
        raise RuntimeError("Asset has no independently comparable SHA256: " + name)
    if asset.get("state") != "uploaded" or not isinstance(asset.get("size"), int):
        raise RuntimeError("Asset is not a complete upload: " + name)
    return {"name": name, "bytes": asset["size"], "sha256": digest[7:],
            "label": asset.get("label"), "contentType": asset.get("content_type")}


def download(asset, destination):
    url = asset["browser_download_url"]
    if urllib.parse.urlsplit(url).hostname != "github.com":
        raise RuntimeError("Unexpected public source asset host")
    for attempt in range(4):
        digest = hashlib.sha256()
        length = 0
        try:
            request = urllib.request.Request(url, headers={"User-Agent": "MyTel-exact-release-migration"})
            with urllib.request.urlopen(request, timeout=180) as response, destination.open("wb") as output:
                while block := response.read(1024 * 1024):
                    output.write(block)
                    digest.update(block)
                    length += len(block)
            expected = asset_identity(asset)
            if length != expected["bytes"] or digest.hexdigest() != expected["sha256"]:
                raise RuntimeError("Source download integrity failure: " + expected["name"])
            return
        except (urllib.error.URLError, TimeoutError):
            if attempt == 3:
                raise RuntimeError("Public asset download failed: " + asset["name"]) from None
            time.sleep(2 ** attempt)


def upload(release, asset, path):
    endpoint = release["upload_url"].split("{", 1)[0]
    parsed = urllib.parse.urlsplit(endpoint)
    if parsed.scheme != "https" or parsed.hostname != "uploads.github.com":
        raise RuntimeError("Unexpected destination upload host")
    query = {"name": asset["name"]}
    if asset.get("label") is not None:
        query["label"] = asset["label"]
    with path.open("rb") as body:
        request = urllib.request.Request(endpoint + "?" + urllib.parse.urlencode(query), method="POST", data=body,
                                         headers={**HEADERS, "Content-Type": asset.get("content_type") or "application/octet-stream",
                                                  "Content-Length": str(path.stat().st_size)})
        try:
            with urllib.request.urlopen(request, timeout=300) as response:
                uploaded = json.load(response)
        except (urllib.error.URLError, TimeoutError) as error:
            # A subsequent manual run safely resumes only matching completed uploads.
            code = getattr(error, "code", "network")
            raise RuntimeError(f"Asset upload failed ({code}): {asset['name']}") from None
    if asset_identity(uploaded) != asset_identity(asset):
        raise RuntimeError("Uploaded destination asset digest/metadata mismatch")
    return uploaded


def inventory(repository):
    releases = pages(f"repos/{repository}/releases")
    for release in releases:
        release["assets"] = pages(f"repos/{repository}/releases/{release['id']}/assets")
    return releases


def signature(releases):
    return {release["tag_name"]: {"name": release["name"], "body": release.get("body") or "",
                                 "targetCommitish": release["target_commitish"],
                                 "prerelease": release["prerelease"], "draft": release["draft"],
                                 "assets": sorted((asset_identity(a) for a in release["assets"]), key=lambda a: a["name"])}
            for release in releases}


def main():
    if not TOKEN or os.environ.get("GITHUB_REPOSITORY") != TARGET or os.environ.get("GITHUB_REF") != "refs/heads/main":
        raise RuntimeError("Run only as the trusted destination main-branch manual workflow")
    source = inventory(SOURCE)
    before = signature(source)
    latest_tag = api(f"repos/{SOURCE}/releases/latest")["tag_name"]
    target = {r["tag_name"]: r for r in inventory(TARGET)}
    if set(target) - set(before):
        raise RuntimeError("Unexpected destination release; review before copying")
    rows = []
    with tempfile.TemporaryDirectory(prefix="mytel-release-copy-") as folder:
        for original in reversed(source):
            tag = original["tag_name"]
            encoded = urllib.parse.quote(tag, safe="")
            source_ref = api(f"repos/{SOURCE}/git/ref/tags/{encoded}")
            target_ref = api(f"repos/{TARGET}/git/ref/tags/{encoded}")
            if source_ref["object"] != target_ref["object"]:
                # URLs differ by repository even when the exact object matches.
                if (source_ref["object"]["sha"], source_ref["object"]["type"]) != (target_ref["object"]["sha"], target_ref["object"]["type"]):
                    raise RuntimeError("Original tag identity was not preserved: " + tag)
            release = target.get(tag)
            if release is None:
                release = api(f"repos/{TARGET}/releases", "POST", {"tag_name": tag, "name": original["name"],
                              "target_commitish": original["target_commitish"],
                              "body": original.get("body") or "", "draft": True,
                              "prerelease": original["prerelease"], "make_latest": "false"})
                release["assets"] = []
            if (release["name"] != original["name"] or (release.get("body") or "") != (original.get("body") or "")
                    or release["prerelease"] != original["prerelease"] or release["target_commitish"] != original["target_commitish"]):
                raise RuntimeError("Existing release metadata differs: " + tag)
            present = {a["name"]: a for a in release["assets"]}
            if len(present) != len(release["assets"]) or set(present) - {a["name"] for a in original["assets"]}:
                raise RuntimeError("Unexpected or duplicate destination asset: " + tag)
            for asset in original["assets"]:
                expected = asset_identity(asset)
                if asset["name"] in present:
                    if asset_identity(present[asset["name"]]) != expected:
                        raise RuntimeError("Existing asset differs; refusing replacement: " + asset["name"])
                else:
                    file = Path(folder) / asset["name"]
                    download(asset, file)
                    present[asset["name"]] = upload(release, asset, file)
                    file.unlink()  # Only this exact verified temporary file; no recursive user-data cleanup.
                print(f"VERIFIED {tag}/{asset['name']} ({expected['bytes']} bytes)", flush=True)
            refreshed = pages(f"repos/{TARGET}/releases/{release['id']}/assets")
            if sorted((asset_identity(a) for a in refreshed), key=lambda a: a["name"]) != sorted((asset_identity(a) for a in original["assets"]), key=lambda a: a["name"]):
                raise RuntimeError("Release asset-set verification failed: " + tag)
            api(f"repos/{TARGET}/releases/{release['id']}", "PATCH", {"draft": original["draft"],
                "prerelease": original["prerelease"], "make_latest": "true" if tag == latest_tag else "false"})
            rows.append({"tag": tag, "assets": len(refreshed), "bytes": sum(a["size"] for a in refreshed)})
    if signature(inventory(SOURCE)) != before:
        raise RuntimeError("Source changed during migration; review before acceptance")
    if signature(inventory(TARGET)) != before or api(f"repos/{TARGET}/releases/latest")["tag_name"] != latest_tag:
        raise RuntimeError("Final release correspondence or latest pointer failed")
    report = {"status": "PASS", "utc": datetime.now(timezone.utc).isoformat(), "source": SOURCE, "target": TARGET,
              "latestTag": latest_tag, "releases": rows, "releaseCount": len(rows),
              "assetCount": sum(r["assets"] for r in rows), "bytes": sum(r["bytes"] for r in rows),
              "tagsPreserved": True, "releaseBodiesPreserved": True,
              "assetNamesSizesLabelsSHA256": "MATCH", "accountPATUsedInRunner": False,
              "platformNote": "GitHub-generated release IDs, uploader identity and creation/publication times are new platform metadata."}
    Path("migration-release-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report), flush=True)


if __name__ == "__main__":
    main()
