# MyTel 3.1.2 Android verification

Application build: `cd68b79a92ebd29ac9c29f199e2a1a99a4962540`, version `3.1.2` / `30102`.
Scope: MyTel Help/About, official website/contact links, removal of Privacy Policy and update controls, neutral examples, and hiding the legacy sign-in navigation row on phone/tablet layouts. SIP/media/chat protocols, package identity, permissions, signing identity and compatibility settings are unchanged.
Mandatory component copyrights, notices, warranty, license text and corresponding-source disclosures remain accessible through Software licenses; source headers remain intact.

Build and artifact checks:

- Clean local attempt `android-build-branding-02` succeeded: 15m45s, 111 tasks (50 executed, 61 up to date), one worker and 1024 MiB heap.
- JUnit: 212 tests, zero failures/errors/skips. Strict lint: zero errors, 189 warnings; the 16 added warnings are unused resources left by removed Help rows/icons.
- [GitHub Actions run 44137124884](https://github.com/mytel-one/MytelPhone/actions/runs/44137124884) succeeded for the application build commit.
- APK: 78,860,356 bytes; SHA-256 `6e3aaeffb75a2124e28b28acc1f2f68646e91c8a123abc799e9b72d0c0a0295e`.
- AAB: 83,159,966 bytes; SHA-256 `53ec11dc7de7e1db1b8bd97593ce93c0130e364eb62cc67b0289d796a1a29ccd`.
- Strict checks passed for production signatures, compressed/extracted JNI, minimum SDK 24, target SDK 37 and all four ABIs: armeabi-v7a, arm64-v8a, x86, x86_64. READ_PHONE_NUMBERS is absent.
- Official Bazaar BIN SHA-256: `d19b5dbcb8d50db620858ac1b560065465edd0a39835d5d86fda2e978dd2d73f`.
- All 13 APKs reconstructed from the BIN passed verification; receipt SHA-256: `f5ad0bfa75e6f589e863cc0c3a57afcffc8f7b05b9ab9fd685aeb845d9df2e02`.

Observed native acceptance:

- Existing D-drive API 31 fixture, successful software-rendered run `branding-02`; production install-r upgraded 30101 to 30102.
- Data inode/UID/GID `123521:10145:10145`, one saved extension-201 account/auth record and the existing Persian chat row were preserved. After-upgrade and restored-state facts passed.
- CUA inspection covered About top/bottom, Diagnostics and the offline Software licenses popup in light and dark themes; inspected pages had readable text and no observed overflow.
- Drawer and notification displayed MyTel. Privacy Policy, update and translation controls were absent from the new Help page.
- The website action emitted ACTION_VIEW for `https://mytel.one/`; Chrome onboarding opened. Browser terms were not accepted, so this is an intent check, not a claim that the page rendered in Chrome. First-party HTTPS routes were checked separately.
- The persisted updater endpoint was empty; current-process startup/Help logs contained no updater markers (`20-no-updater`). This is not packet-capture proof.
- The saved theme setting `1` was restored (`21-restored`); normal app Quit returned to the Android home screen.

Limits: this fixture was isolated and its SIP peer was not started, so it was offline. This presentation release did not repeat call/chat/PSTN tests. During the cold boot of `branding-02`, Android System UI briefly showed a nonresponsive dialog before MyTel started; choosing Wait resolved it, and the app flow completed. Both owned VM runs subsequently closed normally. These results cover the observed environment and do not guarantee every phone or absence of all bugs.
