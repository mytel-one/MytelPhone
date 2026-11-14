MyTel Windows distribution
==========================

Keep `LINPHONEAPP_EXECUTABLE_NAME=mytellphone` and
`LINPHONEAPP_NOTIFICATION_ID=MytellPhone` when updating existing installations.
These retain the profile and Windows notification identity. Set the display name
and vendor with `LINPHONEAPP_APPLICATION_NAME=MyTel` and
`LINPHONEAPP_APPLICATION_VENDOR=MyTel`. Set `LINPHONEAPP_VERSION` to the release
version and `LINPHONEAPP_APPLICATION_URL` to the MyTel repository URL.

The application resource, tray counter and native notification image share the
existing MyTel circular emblem in `Linphone/data/image/logo.svg`. The ICO contains
16, 24, 32, 48, 64, 128 and 256 pixel images of the same artwork. Upstream source
copyright notices and licenses remain included.

Build an installer from a fully deployed and tested flat Windows payload:

```powershell
./cmake/install/windows/Build-MyTelInstaller.ps1 `
  -PayloadDirectory C:/release/MyTel `
  -OutputDirectory C:/release/installer `
  -MakeNsis C:/tools/nsis/makensis.exe -Version 3.0.1 `
  -ApplicationSourceSha <exact-40-character-commit-used-to-build-the-application>
```

Use the official NSIS portable compiler distribution. The helper checks the PE
product/version/publisher and compares the embedded icon to the source artwork
before compiling with warnings treated as errors. The payload must include all
deployed Qt/SDK components, their notices/licenses and supported Microsoft VC++
runtime dependencies. This per-user installer does not install a machine-wide
runtime or change OS notification settings.

The canonical output is `MyTel-v3.0.1-windows-x64-setup.exe`. After successful
compilation, `installer-build-evidence.json` binds its exact size and SHA256 to
`installed-payload.json`, the compiler SHA256, release version and clean desktop
source/archive revision. `applicationSourceSha` separately records the commit
used to compile the application: installer-only changes may advance the archive
revision without rebuilding identical application sources. Release sealing must
reject dirty source evidence and mismatched artifact/manifest hashes. Old build
evidence is removed before a new attempt; changed source or payload during a build
does not receive successful evidence.

MyTel installs to `%LOCALAPPDATA%/Programs/MyTel` by default, registers its own
Installed Apps entry and adds MyTel Start menu and optional desktop shortcuts.
The uninstaller removes only the payload files recorded at build time and empty
installation directories. It keeps the existing user profile, SIP accounts,
contacts and history. It checks that the installed executable is closed before
installing or uninstalling.

Installation accepts a new/empty dedicated directory or an existing MyTel folder
with a matching ownership marker and normalized path. Drive/Windows/system/profile
roots and nonempty unrelated folders are rejected, including silent `/S /D=`
invocations. Uninstall verifies the same marker and path before deleting files.
`tests/mytelv3/installer_guard_test.ps1` executes the production NSIS guard and
sections against a small payload. It isolates registry/shortcut names with a
unique test suffix and probes real root validation in a section that quits before
any payload write.

`Assert-MyTelBranding.ps1` also checks the final application/installer PE directly.
The small `mytel-resource-fixture` target in `tests/mytelv3` compiles the actual
production resource template without rebuilding the SIP engine; it does not
replace testing the final deployed EXE, native toast and installer UI.
