# MytellPhone v3 regression evidence

These journeys were derived from the supplied handoff, source inspection and
the user's request. Credentials and user configuration are excluded.

## Commands

- `node --test --test-reporter=tap tests/mytelv3/*.test.cjs`
- `tests\mytelv3\build.cmd` (installed Visual Studio 2022 and Qt 6.10)
- The existing `mytel-chip-test` target continues to load the production chip.
- Full application and PBX/device acceptance are recorded by the release owner.

## Reproduced failures

| Journey | RED evidence | Fix and GREEN guarantee |
| --- | --- | --- |
| Dial/chat an extension, Persian number or SIP destination | 39 of 78 initial production-entry tests failed | All direct call/chat entries use one validated normalizer |
| Call a contact with the same number rules | 9 of 104 tests failed when contact dialing was added | Contact, call-page, call-form and chat-form entries now agree |
| Switch theme without restarting | JS style test failed after palette change | Style accessors update live QML bindings |
| Read the connection state | Both contrast tests failed | Online/offline text has at least 4.5:1 white-text contrast |
| Cancel while registration completes | Required production attempt gate was missing (C1083 compile RED) | Shared atomic gate admits acceptance or cancellation; 502 ordering/race scenarios pass |

The Qt harness instantiates the production `DefaultStyle.qml`, `Themes.qml`,
`AppIcons.qml` and `buttonStyle.js`. It switches light → dark → light → dark,
checks real QML property notifications/bindings, and measures ordinary text on
the application surface: 6.46:1 light and 10.90:1 dark. Accent labels remain white.
It uses isolated test preferences.

Additional production-function tests check that dialing uses the current input
rather than the history debounce, rejects invalid values without closing the
keypad, and clears both displayed/search values immediately. QML syntax for all
22 changed/new QML files was checked with Qt 6.10 `qmlformat`: zero failures.

## Limits

The unit checks do not establish SIP/RTP delivery, microphone/speaker quality,
camera quality, external trunk access, or Android behavior. Those require the
separate runtime acceptance matrix. No whole-application coverage percentage is
claimed; this repository has no configured full application coverage target.
