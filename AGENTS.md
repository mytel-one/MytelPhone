# MytelPhone development destination

- The canonical project repository is `https://github.com/mytel-one/MytelPhone`.
- All new development and releases belong there. `DashSaman/MyTelSoftPhone` is the historical source; do not push new work to it.
- Android development uses `main`. Native Windows development uses the separate `windows-v3` branch. Do not merge the unrelated Windows tree into Android `main`, and never push MyTel changes to Belledonne's upstream repository.
- Preserve historical tags, commits, release assets, checksums, package identifiers, signing identities, and account/profile data. Historical verification links may intentionally reference the old repository and its CI runs.
- On this Windows workstation, use `python tools/github_mytelphone.py <gh arguments>` for project GitHub CLI operations. It obtains the `mytel-one/MytelPhone.git` credential from Git Credential Manager and supplies it only to that subprocess. The existing global `gh` account belongs to other work and must not be silently switched.
- Git `origin` must point to `https://github.com/mytel-one/MytelPhone.git`; legacy/upstream remotes are fetch-only. Never put a token in a URL, repository file, command argument, log, or committed configuration.
- Keep strict Android unit/lint/build checks enabled. CI debug APK uploads are opt-in on a manual run and expire after three days; publish distributable packages as Releases.
- The historical-release copier is a destination-main-only manual migration utility. It uses the workflow's short-lived `GITHUB_TOKEN`, preserves binary digests and tag identity, and must refuse conflicting existing assets.
