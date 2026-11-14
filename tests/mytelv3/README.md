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
| Type on an open dial pad | 5 of 8 keyboard cases failed | Top-row digits and `#` work; Ctrl shortcuts remain available |
| Type a transfer destination | 1 of 3 cases failed: it placed a second call | Transfer requests target the active call; ordinary call mode is preserved |
| Drop an attachment with no sharing service | 1 of 3 cases failed | No attachment is submitted without an explicitly configured service |
| Choose contact destinations | Exact production method compiled: only 4 of 7 cases passed | SIP usernames take precedence over matching phone numbers; unrelated numbers/order and original lists stay intact (7/7 pass) |
| Remove contact numbers/addresses | Exact methods in isolated child processes: 3/10 passed; invalid indexes crashed or marked unchanged contacts dirty | Negative, at-size and empty-list removals do nothing; valid removals notify once, mark unsaved, and clear only a removed default address (10/10 pass) |
| Open/reset an empty conversation | Four production binding/handler cases threw on missing chat/message objects | All four null-state cases pass |
| Read call labels and controls after switching theme | 8 of 12 production color-binding cases failed in the old palette | Dedicated media surfaces keep white call labels and icons readable in both modes (12/12 pass) |
| Read history/contact detail cards | 12 of 16 color/accessibility cases failed, including the white history card with pale dark-mode labels | Detail cards, contact fields/banner/note and the creation gradient use adaptive surfaces; ordinary and missed-call dates retain text contrast (16/16 pass) |
| Open call/main options and account popups | 8/16 production surface checks failed; native Qt popup palette failed all four theme switches | Shared popup surfaces and incoming notification contrast pass 16/16; native Qt popup/text-field colors inherit the actual window palette on every switch |
| Return from adding an account | 5/6 navigation cases failed; seven global routes were absent from the footer wrapper | Adding an account retains the main page, Back restores it, and all seven wrapper routes forward to the retained layout (13/13 pass) |
| Read dial codes under a Persian keyboard | Qt renders neutral `*43` as `43*`; left alignment alone does not change its bidi base | Exact production public-Qt helper corrects four code formats in enabled/disabled fields, preserving raw text, cursor, selection, width binding and subsequent edits (8/8 pass) |

The Qt harness instantiates the production `DefaultStyle.qml`, `Themes.qml`,
`AppIcons.qml` and `buttonStyle.js`. It switches light → dark → light → dark,
checks real QML property notifications/bindings, and measures ordinary text on
the application surface: 6.46:1 light and 10.90:1 dark. Accent labels remain white.
It uses isolated test preferences.
The harness also instantiates a Qt Quick button with the production call style:
text contrast stays 9.37:1 on controls, 6.14:1 on hover, 18.16:1 on the stage,
and 12.74:1 on participant tiles across all four theme switches. Actual video
rendering and call-window appearance remain part of device acceptance.
It extracts the production `AbstractWindow` palette bindings into a real Qt
ApplicationWindow and checks a Basic Popup/Label and TextField: ordinary field
text measures 6.07:1 light and 12.06:1 dark, and popup surfaces/labels match the
selected application palette without restarting.

The contact harness compiles the exact `FriendCore::getAllAddresses()` and
`Utils::getUsername()` bodies extracted at configure time, using small Qt-only
class seams. It reproduced the discarded `std::remove_if` return value causing
duplicates and moved-from blank entries. `QList::removeIf` fixes that behavior.
The removal harness likewise compiles the exact production bodies and runs each
case in a child process, so an access violation is reported as a failed case.
The chat layout harness instantiates the exact production composing-header and
editor blocks with small view-dependency seams. The old editor block from
`096571c` produces a `contentWidth` binding-loop warning; the corrected editor
and composing header produce zero warnings in the harness. Whole application
logs and visible behavior still require separate runtime acceptance.
The direction harness injects an RTL/Persian keyboard through a private Qt
test seam. Production uses only the public direction-key event and bindable
width API; private Qt headers are excluded from the application. Destination
fields request Latin input without rejecting Persian digits. Chat composition
retains its existing direction handling.

Additional production-function tests check that dialing uses the current input
rather than the history debounce, rejects invalid values without closing the
keypad, and clears both displayed/search values immediately. The final Node suite
contains 189 passing cases. Changed QML files were checked with Qt 6.10
`qmlformat`: zero syntax failures.

## Limits

The unit checks do not establish SIP/RTP delivery, microphone/speaker quality,
camera quality, external trunk access, or Android behavior. Those require the
separate runtime acceptance matrix. No whole-application coverage percentage is
claimed; this repository has no configured full application coverage target.
