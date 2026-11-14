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
  -MakeNsis C:/tools/nsis/makensis.exe -Version 3.0.1
```

Use the official NSIS portable compiler distribution. The helper checks the PE
product/version/publisher and compares the embedded icon to the source artwork
before compiling with warnings treated as errors. The payload must include all
deployed Qt/SDK components, their notices/licenses and supported Microsoft VC++
runtime dependencies. This per-user installer does not install a machine-wide
runtime or change OS notification settings.

MyTel installs to `%LOCALAPPDATA%/Programs/MyTel` by default, registers its own
Installed Apps entry and adds MyTel Start menu and optional desktop shortcuts.
The uninstaller removes only the payload files recorded at build time and empty
installation directories. It keeps the existing user profile, SIP accounts,
contacts and history. It checks that the installed executable is closed before
installing or uninstalling.

`Assert-MyTelBranding.ps1` also checks the final application/installer PE directly.
The small `mytel-resource-fixture` target in `tests/mytelv3` compiles the actual
production resource template without rebuilding the SIP engine; it does not
replace testing the final deployed EXE, native toast and installer UI.
