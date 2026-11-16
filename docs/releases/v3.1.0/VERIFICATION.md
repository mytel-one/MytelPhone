# MyTel 3.1.0 verification

Status: validation in progress. This record must be completed before publication.

## Change scope

Android and Windows now have a compact three-field SIP setup with optional advanced connection settings, automatic transport selection, readable light/dark controls and clearer connection feedback. Appearance and background-access settings are easier to reach. Internal executable, profile, notification, Android package and production signing identities remain stable.

Android's minimum supported version is API 24 (Android 7), with ARMv7, ARM64, x86 and x86_64 native libraries. Java-time desugaring and explicit API guards cover older notification, Telecom, image, clipboard and picture-in-picture paths. An App Bundle accompanies the universal APK for store delivery. Optional camera, Bluetooth, Wi-Fi and telephony hardware does not become an unnecessary install filter.

Runtime review also reproduced failed-setup credential retention, incorrect first microphone-permission handling, a persistent connected-call heads-up and a recording duration affected by Tehran's half-hour timezone offset. Fixes are being validated against these reproductions.

## Automated checks

| Check | Result |
|---|---|
| Final Android unit suite, strict lint and signed release/R8 APK+AAB | Pending final clean build |
| APK/AAB package/version/minimum SDK/signing continuity | Pending final artifacts |
| Four-ABI JNI inventory, 64-bit ELF page alignment and APK ZIP alignment | Pending final artifacts |
| AAB `PAGE_ALIGNMENT_16K` and generated device APKs | Pending final artifacts |
| Windows application regression tests | 225 passed |
| Windows setup/navigation tests | 45 passed |
| Production-QML component assertions | 96 passed |
| Real SDK SIP address/transport scenarios | 188 passed |
| Real SDK authentication ownership/persistence scenarios | 16 passed |
| Isolated installer contract, including installed project URL | 13 passed |

The production Android certificate fingerprint is `60ebd14405aeb1fe58e277297182fb48cb77d07f8046982a07e494ba49b4d7f4`. Published compatibility reports bind their results to the final artifact hashes. They do not replace runtime acceptance.

## Runtime matrix

| Environment | Scope | Result |
|---|---|---|
| Android 7/API 24, x86_64, 4 KB | Prototype fresh install, setup failure/retry/cancel/403/Online, contacts, UTF-8 messages, incoming/outgoing call, hold/resume, native RTP and recording | Passed; final regression pending |
| Android 7/API 24, x86 | Forced 32-bit native startup, retained lab contacts/messages and Online registration | Prototype passed; final regression pending |
| Android 17/API 37, x86_64, 16 KB | Final native startup, permissions, setup, call/chat and display | Pending |
| Android 12/API 31 | Existing signed 3.0.1 to signed 3.1.0, account/message/contact preservation | Pending |
| Windows x64 | Full native application, light/dark setup/main/settings, local SIP/media, profile preservation and installer | Pending |

The old 3.0.1 APK failed installation on the API 24 x86_64 environment with `INSTALL_FAILED_NO_MATCHING_ABIS`; the new prototype installed and ran. This emulator's initial kernel entropy starvation was corrected in the isolated test environment using host-generated cryptographic entropy. No application workaround replaced secure randomness.

The test fixtures use fictional accounts and bounded local SIP/RTP traffic. Native call state, exchanged audio packets and tone generation establish protocol/media-path evidence. They do not establish microphone/speaker quality on a physical handset. Prior 3.0.1 physical Android/Windows/PSTN feedback is historical evidence, and is not represented as a fresh 3.1.0 physical-device test.

## Preservation and package provenance

Only final clean source revisions may produce the release artifacts. The release manifest records separate Android and Windows application build revisions, the complete source archive revisions and their hashes. Post-build source changes are limited to reviewed documentation and installer/release-support paths; application changes require rebuilding.

Windows lab testing uses a closed-application full-profile snapshot and a separately held original directory. File hashes and backing file IDs distinguish the native Windows profile from Codex's separate MSIX view. They are never merged. Private profiles, databases, recordings, keys and test logs are excluded from public runtime/source packages.

The existing SDK source ZIP and manifest remain byte-identical historical assets. Their disclosed missing external source pins and unverified prebuilt-dependency correspondence remain unresolved. This release does not claim a complete offline SDK rebuild. Windows application/installer binaries are not Authenticode-signed.

## Limits

Passing the tested matrix does not guarantee every phone, PBX, network or codec combination. Android versions below 7 and unsupported CPU architectures are outside this package's install contract. ARM libraries receive artifact checks; physical ARM handset testing must be distinguished from x86/x86_64 emulator testing.

SIP SIMPLE text delivery, external calling, transfers, video, push and background reception depend on compatible PBX/provider configuration, permissions and network reachability. A SIP server acknowledgement is not a read receipt. The application supplies no PSTN subscription or universal push service. This record must state explicitly which of these paths were actually exercised in the final acceptance run.
