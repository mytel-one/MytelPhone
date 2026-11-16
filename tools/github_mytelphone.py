"""Run gh for MytelPhone with its repository-scoped Windows Git credential.

No token is stored in this file, a remote URL, or subprocess command arguments.
The existing global gh account remains unchanged.
"""
from __future__ import annotations

import os
import shutil
import subprocess
import sys

REPOSITORY = "mytel-one/MytelPhone"
ACCOUNT = "mytel-one"


def main() -> int:
    args = sys.argv[1:]
    if not args or args[0] in {"auth", "config", "extension", "alias"}:
        raise SystemExit("Use this wrapper for repository operations, not credential/configuration commands.")
    environment = os.environ.copy()
    for key in ("GH_DEBUG", "GIT_TRACE", "GIT_TRACE_CURL", "GIT_CURL_VERBOSE", "GCM_TRACE"):
        environment.pop(key, None)
    environment["GIT_TERMINAL_PROMPT"] = "0"
    environment["GCM_INTERACTIVE"] = "never"
    query = f"protocol=https\nhost=github.com\npath={REPOSITORY}.git\nusername={ACCOUNT}\n\n"
    result = subprocess.run([
        "git", "-c", "credential.https://github.com.helper=",
        "-c", "credential.https://github.com.helper=manager",
        "-c", "credential.useHttpPath=true", "-c", "credential.interactive=false",
        "credential", "fill"], input=query, text=True, capture_output=True,
        env=environment, timeout=30)
    credential = dict(line.split("=", 1) for line in result.stdout.splitlines() if "=" in line)
    if result.returncode or credential.get("username") != ACCOUNT or not credential.get("password"):
        raise SystemExit("The MytelPhone repository credential is unavailable in Git Credential Manager.")
    gh = shutil.which("gh")
    if not gh:
        raise SystemExit("GitHub CLI is not installed.")
    environment["GH_TOKEN"] = credential["password"]
    environment["GH_REPO"] = REPOSITORY
    environment["GH_PROMPT_DISABLED"] = "1"
    return subprocess.run([gh, *args], env=environment).returncode


if __name__ == "__main__":
    raise SystemExit(main())
