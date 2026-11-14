# Server directory regressions

The harness compiles the production `ServerDirectoryManager.cpp`, its real Qt
network implementation and Qt signal/event-loop code. Only the unrelated
Linphone core/config boundary is replaced with an in-memory test config.
Each scenario runs in a child process with a three-second watchdog, so a
deadlock fails rather than hanging the test runner.

Run from an x64 Visual Studio developer command prompt with Qt 6.10 on PATH:

```bat
cmake -S tests/myteldirectory -B build-directory-tests -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.10.0/msvc2022_64
cmake --build build-directory-tests
build-directory-tests\mytel-directory-test.exe
```

Baseline evidence before the fix:

```text
cache-search: FAIL (hung)
invalid-url: FAIL
invalid-cache: FAIL (hung)
stream-limit: FAIL (hung)
```

The baseline stream supplied a Content-Length and held its socket open after
600,000 bytes. The final regression additionally omits Content-Length: the
manager must abort during receipt, without waiting for the peer to finish.

After the fix all four scenarios pass:

- Search returns cached contacts, preserves leading zeros, matches names
  without case sensitivity and stops exposing the old cache after a URL change.
- An insecure replacement URL clears the selected source.
- A mixed cache containing a numeric `number` is rejected as a whole.
- A stream exceeding 512 KiB is aborted within the watchdog deadline, even
  without Content-Length and without closing the server socket.

These tests do not establish PBX connectivity or audio/video quality.
