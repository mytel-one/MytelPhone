# MyTel 3.1 Windows setup and navigation checks

The setup screen presents extension, password and server first. Optional transport,
authentication, registrar and proxy fields stay behind Connection options. The
existing MyTel navy, blue and orange identity and the application's English UI are
retained. Calls and chats expose labelled creation buttons; settings categories
explain their purpose and skip unavailable pages.

The visual contract is a calm, legible work phone with one primary action per
setup state, 44 px input targets, clear field errors, cancellable registration and
consistent light/dark surfaces. The compact layout is checked at 1020x700; the
brand pane appears at 1512x1007. Optional fields scroll when they exceed the compact
viewport. Existing account identities, history and stored transport settings are
not migrated by this change.

## Run

From the Windows checkout, with Node, Qt 6.10 MSVC and Visual Studio installed:

```powershell
node --test tests/mytelv310/*.test.cjs
node --test tests/mytelv3/*.test.cjs
tests/mytelv310/build-ux.cmd C:/path/to/evidence
tests/mytelv310/build-address.cmd
```

The small native test scripts use the existing temporary theme-test build tree,
build their named target with one worker and do not build or launch the application.
The SDK address script uses the adjacent `win/sdk/linphone-sdk/win64` installation
and runs from its resource directory so that the real SIP grammar is available.
It creates Address objects only: no Core, account registration or network calls.

## Recorded results

| Check | Result |
| --- | --- |
| New setup validation, settings navigation, transport and ComboBox model guards | 45/45 |
| Existing Node regressions | 225/225 |
| Production QML setup/control render and interaction seam | 96/96 |
| Real installed Linphone SDK Address parsing, route policy and identity construction | 188/188 |
| Changed QML parsed by Qt qmlformat | 8 files |

RED checkpoints retain the observed failures before the fixes: `red.log`,
`transport-red.log`, `combo-red.log`, `address-red.log`, `qt-label-red.log`,
`routes-red.log`, `route-mode-red.log`, `qt-route-mode-red.log` and
`route-parameter-case-red.log`.
The corresponding GREEN outputs are included alongside the tests. Test counts are
assertions/scenarios, not whole-application coverage measurements.

Automatic transport applies only to new setup: SIPS always selects TLS; otherwise
an explicit UDP/TCP/TLS choice wins, followed by a SIP URI transport parameter,
then TLS for an explicit port 5061 and UDP for an ordinary server. Bracketed IPv6
ports are supported without treating an unbracketed IPv6 suffix as a port.
Automatic is passed separately to the native boundary so each registrar/proxy
resolves its own URI transport and port. An explicit UI choice wins for non-SIPS
addresses. Any main server or advanced route using SIPS forces TLS on the complete
setup while retaining each URI's original scheme. Case-variant transport parameter
names are canonicalized using their SDK-parsed values before choosing the transport.
The appended Automatic argument defaults to false for existing explicit API callers.

The native AccountManager boundary uses the SDK parser for SIP/SIPS, bare host/port
and bracketed IPv6. Identity host and port come from the parsed Address. SIPS stays
secure even if a caller or URI requests UDP/TCP. The SDK's `setTransport()` ignores
SIPS, so its secure URI transport parameter is explicitly canonicalized to TLS.
Invalid/non-SIP inputs return a clear setup error without changing stored accounts.

## Scope and remaining acceptance

The Qt harness loads the production setup, controls, themes and settings menu item,
with a registration backend double and temporary QSettings. It exercises required
field focus, keyboard progression, automatic and explicit transport, progress,
cancellation, simulated registration success, both themes, and optional field
reachability. It does not contact a PBX or test real registration. The settings
sidebar picture uses a representative fixture, not the full SettingsPage.

The offscreen backend does not draw the application's MultiEffect icon shaders;
their actual appearance needs a native GPU-backed run. Full Calls/Chats/settings
navigation, installer screens, actual account registration and preservation of the
production profile are separate native acceptance gates. No installed production
process or profile is touched by these tests.

The application has no active `--config`, `--profile` or multi-instance option.
Its single-instance mode excludes executable path/version, and `--fetch-config`
merges into the active profile. Use a separate Windows user or a closed-profile
backup/swap/restore procedure for native first-run acceptance; a staged executable
path alone does not isolate profile data.
