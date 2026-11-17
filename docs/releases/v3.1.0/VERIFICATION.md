# MyTel 3.1.0 verification

Status: the build, packaging, native runtime, API 31 upgrade and actual Windows installation checks below are complete and passed. Final integrity sealing, publication and verification of the published bytes remain separate stages. No publication is claimed here.

## Artifact identity and install contract

| Compiled application | Clean build commit |
|---|---|
| Android APK and AAB | `d7cb149cb8033f9f07e2ea786ff3a8d973baa651` |
| Windows application and installer | `432520db8257ab7a09d7ae79eddd0b432799e2fc` |

| Artifact | Bytes | SHA256 |
|---|---:|---|
| Android release/R8 APK | 178,544,800 | `4b508dd6cc229d774f5c889967f75814863de0279d5d9f3edc660ddf4b6be835` |
| Android release AAB | 83,145,214 | `3bf7861e9dc42588925f5329c2c3d9259ac0a42e4696cf4801fbe9025c2e99c6` |
| Windows x64 application | 82,745,856 | `62af966cec9cc9c73a559a0453510eb5d4aec9f661c20dd017371af3b95a9d93` |
| Windows x64 installer | 105,796,156 | `643cc3400ccb5a92508ccef7b1ff8568ca0ecb08f3e63bcd7ef4d7a8bfdef24e` |

The Android package is `one.mytel.softphone`, version 3.1.0 / code 30100, with **minSdk 24 (Android 7), targetSdk 37**, and `armeabi-v7a`, `arm64-v8a`, `x86` and `x86_64` libraries. The universal APK/AAB distribution checks passed: production signing continuity, native ELF alignment, APK ZIP alignment and AAB `PAGE_ALIGNMENT_16K`. Production certificate SHA256 is `60ebd14405aeb1fe58e277297182fb48cb77d07f8046982a07e494ba49b4d7f4`.

These are compiled build revisions. A later documentation/source-archive/tag revision must be recorded separately in the final release manifest; a documentation commit does not change the identity of the tested binaries.

## Automated checks

| Suite | Recorded result |
|---|---|
| Final Android unit suite | **204 tests in 22 XML reports; 0 failures, errors or skipped tests** |
| Final Android strict lint | **0 errors; 173 reviewed warnings** |
| Signed Android release/R8 APK, AAB and artifact-bound distribution checks | Passed |
| Windows existing application regressions | 225 passed |
| Windows setup/navigation checks | 45 passed |
| Production-QML setup/control assertions | 96 passed |
| Real SDK SIP address/route scenarios | 188 passed |
| Real SDK authentication ownership/persistence | 16 passed |
| Production account cleanup/destructor and prior-history scenarios | 22 passed |
| Real SDK chat identity migration, history, rollback and restart checks | 193 passed |
| Isolated installer contract | 13 passed |
| Production toast XML cases | 14 passed after 9 baseline failures |
| Production pre-dial popup with real Qt controls, both themes | 23 passed after 12 baseline failures |

Counts belong to their individual suites and are not added into a combined coverage figure. Focused Android regressions are already included in the 204-test total. Actual installation has its own acceptance below, separate from the installer-contract tests and packaging checks.

## Native acceptance of the current builds

| Environment | Observed scope and result |
|---|---|
| Final Android, API 37/x86_64, 16 KB | Production device-selected splits ran through `app_process64` with 13 executable native mappings. The selected variant reports minSdk 32; the universal install floor remains 24. |
| Final Android conversation | Copy/delete/paste/send and new incoming messages stayed unique in the live view and after reopening. Fresh messages each appeared once on the wire; the pagination sweep covered **41/41 unique messages with no visible duplicates**. No storage reset was used. |
| Final Android local SIP/media | Before the call, 119 MESSAGE requests and no INVITE were recorded. The later PCMU call completed INVITE/ACK/BYE with **1,952 RTP packets received and 1,700 sent**, no fixture socket resets or recorded errors. |
| Final Android, API 24/x86, 4 KB | Actual `app_process32` with 12 executable native mappings; real PBX registration/messages/calls and retained account, contact/history and encrypted-recording data. Physical ARM execution is not established by this emulator result. |
| Production-signed 3.0.1 to final 3.1.0, API 31 | **Passed using only package install-r:** versionCode 30001 to 30100, same application data-directory inode/UID. The account/auth records, UDP transport and nonempty credential-field hashes matched; dark mode, two exact Persian messages, the native contact-provider row and both independent call-history records remained. The prior failed outgoing call and prior connected outgoing call stayed distinct. Native contact visibility and Online registration passed. |
| Final Windows x64 lab profile | Light/dark pre-dial destination, Call/Cancel labels and native Cancel flow passed. Persian messages and a contact name containing `&` were retained. The contact popup started a connected call to the intended lab SIP peer using G.711u. At 41 seconds Windows RX/TX were **1,169/1,067**, with reciprocal peer counters. Correct call-window hangup left zero test channels; normal Quit completed. |
| Installed Windows x64, original profile | The installed application opened the retained initial setup draft; this original profile had no configured account. Dark-theme Automatic/UDP/TCP/TLS choices were readable. Restored-account Online recovery is not claimed. |

The final Android screenshots were checked visually and the six-image CafeBazaar gallery was prepared. Store upload has not occurred.

The API 37 cold-boot System UI ANR recovered through Wait; MyTel startup remained healthy. The bounded API 24 emulator entropy correction was test infrastructure using host-generated cryptographic entropy; no application change replaced secure randomness.

The API 31 upgrade used the official x86_64 emulator image: the old ARM64 application ran through native translation; the final application ran natively as x86_64 with 13 executable JNI mappings. No uninstall, storage clear, data restoration or AVD recreation occurred during the upgrade. This is emulator upgrade evidence, not physical ARM-device coverage.

## Earlier, separately identified runtime evidence

Windows revision `d500b65659ed77e7f06053c033714f31b964e4c7` interoperated with the final Android build on an isolated real PBX. Registered contacts, native Persian messages in both directions, incoming/outgoing answered calls, Windows hold/resume and normal hangups passed. Reciprocal RTP counters grew by 1,570 packets on the outgoing path, 323 packets over 6 seconds after resume, and 318 packets over 6 seconds on the reverse path. Initial unanswered attempts are excluded from those successful-call results.

**One Asterisk TX lost packet was reported on a held/resumed leg.** Zero packet loss or physical speech quality is not claimed. Registered contacts were not OPTIONS-qualified, and the native message observation is not a captured MESSAGE/200 exchange or a read receipt.

On that Windows revision, OS history contained exactly one own message toast preserving a Persian contact title, literal `&`, `<test>` and `]]>`, and the MyTel logo. The incoming call window opened and was answered, but its separate OS call-toast history query returned **`UNVERIFIED_NO_MATCH`**. A standalone call-toast acceptance is therefore not claimed.

Earlier Android revision `b482d146a69de0f6bbca64d54a288554f66b1b9e` passed 32-bit API 24 startup/encrypted SAF export and API 37 existing-account LAN denial/retry checks. Revision `bf25ac566024c6c612b290096c34f1eb405e0a8c` separately passed encrypted SAF Cancel/rotation/Save, modern MediaStore export, one-action LAN setup continuation and elapsed-time preview checks. Original encrypted files remained unchanged and owned temporary plaintext was cleaned. These results remain attached to those revisions; they are not described as reruns on the final chat-fix binary.

The earlier transient duplicate-message display was reproduced on b482 and then resolved and natively replayed on the final Android build. The earlier empty/raw-key pre-dial popup was reproduced on d500 and then resolved and natively replayed on the final Windows build.

## Preservation, actual installation and remaining gates

Temporary PBX configuration has been cleaned and runtime reconciliation reached **`CLEANED_RUNTIME_VERIFIED`**. Unrelated configuration bytes were preserved and no PBX restart was performed.

The original Windows profile was restored with all eight backing file IDs and hashes verified. The separate ten-file MSIX view was unchanged; registry and shortcut restoration checks passed. These distinct profiles were not merged.

Actual Windows installation passed with status **`INSTALLED_VERIFIED_BEFORE_FIRST_APP_LAUNCH`**, installer exit 0 and **1,523 payload files verified**. Before first application launch, the original profile IDs/hashes and separate MSIX view matched their pre-install state. App/uninstaller icons, installation/uninstallation metadata, three expected shortcuts, the preserved legacy shortcut root, protected registry and the original full installation backup passed their checks.

An earlier preflight refused before installer launch. The workspace-only verifier was corrected to use the verified PowerShell 7 runtime instead of the failing Windows PowerShell path; no application or packaged-artifact changes were needed. The earlier refusal is not counted as an installation.

The actual MyTel uninstaller wizard displayed the MyTel title, logo, `mytel.one` website and data-preservation text. Native Cancel closed it; a separate check after that observed Cancel confirmed all 1,523 installed payload files unchanged. The premature check taken before Cancel is excluded from that result. No uninstall was performed.

The tested API 31 upgrade and Windows installation gates are complete. Remaining work is final source/archive provenance and integrity sealing, publication, and anonymous verification of the published files. No release or store publication is claimed here.

## Limits and source correspondence

The install contract starts at Android 7/API 24 and covers the four packaged ABIs. Artifact checks on ARM libraries do not establish physical ARM handset or every-phone compatibility. No fresh 3.1.0 physical-handset/PSTN speech-quality, video, transfer/conference, Bluetooth or attachment-network round trip is claimed. Native RTP counters demonstrate packet flow, not microphone/speaker quality.

Windows binaries are not Authenticode-signed; clean-machine acceptance is unverified. The inherited SDK source ZIP/manifest remain byte-identical historical assets, with 26 unavailable exact external source pins and unverified prebuilt-dependency correspondence disclosed separately. A complete offline SDK rebuild is not established. Private credentials, profiles, recordings and test captures are excluded from public packages.
