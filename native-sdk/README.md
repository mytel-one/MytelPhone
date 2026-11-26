# Android SIP SDK for MyTel 3.2.1

The application uses the Linphone Java API and resources from upstream version `5.6.0-alpha.190+026b4e9992`. MyTel's native change sends responses through the open reliable SIP connection on which the corresponding request arrived. This handles TLS or TCP calls where the peer's Via advertises a different listening port.

## Build the SDK

Use a Linux or macOS Android build host with the upstream prerequisites. Check out the Linphone SDK and its pinned submodules at commit `026b4e99928da77a75f7852dd5c8631bd1a3ae17`. Use Android NDK r30 (build 16248370) and native API 21. Apply `native-sdk/reliable-sip-return.patch` from this application at the SDK root. The patch SHA-256 is `18645df2437b1a422951b68eb369edb2d47eb690cdc18f77cd5c63b346a25ed7`.

The upstream source is maintained in the [Linphone SDK repository](https://github.com/BelledonneCommunications/linphone-sdk). The exact dependency commits and public source locations are listed in [upstream-gitlinks.tsv](upstream-gitlinks.tsv). The MyTel native loopback controls can be run with `bash native-sdk/regression/run-native.sh <sdk-checkout> <new-test-directory>`.

Build the regular Android SDK configuration with Ninja in RelWithDebInfo. For the four preset names use `android-arm64`, `android-armv7`, `android-x86` and `android-x86_64`. Give each ABI its own build and install directory. The configuration includes the following version and platform arguments.

```sh
cmake --preset=android-arm64 -B build-arm64 -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DLINPHONESDK_VERSION=5.6.0-alpha.190+026b4e9992 \
  -DLINPHONESDK_STATE=snapshots -DLINPHONESDK_BRANCH=master \
  -DCMAKE_INSTALL_PREFIX="$PWD/install-arm64" -DANDROID_PLATFORM=android-21
cmake --build build-arm64 --target liblinphone
```

After stripping with the pinned NDK, put each verified `liblinphone.so` under `<engines>/<Android ABI>/`. Check JNI exports, runtime dependencies, page alignment and the incoming TLS regression before recording release hashes. The public upstream AAR used as the packaging base has SHA-256 `595ba9d1e34623595c0fe69df230263af6dbcdd46ddea6f9c823868ba6b28eab`. That AAR's POM has no transitive dependencies.

## Build the application

A normal checkout resolves the released `MyTel-sip-sdk-3.2.1.aar` from the public MyTel Android 3.2.1 release. To build with a local copy, pass its absolute path as `-Pmytel.sdkAar=/path/to/MyTel-sip-sdk-3.2.1.aar`. Gradle verifies the archive and the native engines against the SHA-256 pins in this directory before building.

To assemble an AAR from the verified native engines, run the following command with Python 3.11 or newer from this application repository.

```sh
python tools/prepare_mytel_sdk.py --upstream /path/to/upstream.aar \
  --engines /path/to/engines --output /path/to/MyTel-sip-sdk-3.2.1.aar
```

The arm64 cellular acceptance build uses an AAR produced with `--arm64-device-test`. Pass that AAR with `-Pmytel.sdkAar`, enable `-Pmytel.arm64DeviceTest=true`, and run `:app:assembleDeviceTest`. This selects the signed single-ABI device-test variant. Use `:app:assembleRelease :app:bundleRelease` with the four-ABI SDK for distribution.

Release signing reads the existing local `keystore.properties`. A separate build checkout can point to this private file with `-Pmytel.signingProperties=/path/to/keystore.properties`. Relative store paths are resolved against the accompanying `app` directory. Keep signing files outside version control.
