#!/usr/bin/env bash
set -euo pipefail

abi="${1:?provide Android ABI}"
case "$abi" in
    arm64-v8a)     preset=android-arm64 ;;
    armeabi-v7a)   preset=android-armv7 ;;
    x86)           preset=android-x86 ;;
    x86_64)        preset=android-x86_64 ;;
    *)             exit 2 ;;
esac

app_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="${RUNNER_TEMP:?}/mytel-android-sdk-$abi"
mkdir -p "$work/engine"
cd "$work"
pin=026b4e99928da77a75f7852dd5c8631bd1a3ae17
source_patch="$app_root/native-sdk/reliable-sip-return.patch"
printf '%s  %s\n' 18645df2437b1a422951b68eb369edb2d47eb690cdc18f77cd5c63b346a25ed7 "$source_patch" | sha256sum --check

git init --quiet sdk
git -C sdk remote add origin https://github.com/BelledonneCommunications/linphone-sdk.git
git -C sdk fetch --no-tags --depth=1 origin "$pin"
git -C sdk checkout --quiet --detach FETCH_HEAD
test "$(git -C sdk rev-parse HEAD)" = "$pin"
git -C sdk submodule update --init --depth=1 --jobs=4
git -C sdk/external/mbedtls submodule update --init --depth=1 framework
git -C sdk apply --check "$source_patch"
git -C sdk apply "$source_patch"

# The distributor's exact Linux NDK archive and checksum are pinned alongside the SDK inputs.
curl --fail --location --retry 3 --retry-all-errors -o ndk.zip \
    https://dl.google.com/android/repository/android-ndk-r30-linux.zip
printf '%s  ndk.zip\n' 753611f410d002cfcd3f3dc2ef49aad532089d3180b436c060a90bf0fcb64df2 | sha256sum --check
unzip -q ndk.zip
rm -- ndk.zip
export ANDROID_NDK_HOME="$work/android-ndk-r30" ANDROID_NDK_ROOT="$work/android-ndk-r30"
export JAVA_HOME="${JAVA_HOME_17_X64:?}"
export PATH="$JAVA_HOME/bin:/usr/bin:$PATH"
export ANDROID_SDK_ROOT="${ANDROID_HOME:?}"
tools="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin"

cmake --preset="$preset" -S "$work/sdk" -B "$work/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLINPHONESDK_VERSION=5.6.0-alpha.190+026b4e9992 \
    -DLINPHONESDK_STATE=snapshots -DLINPHONESDK_BRANCH=master \
    -DCMAKE_INSTALL_PREFIX="$work/install" -DANDROID_PLATFORM=android-21
cmake --build "$work/build" --target liblinphone --parallel 2
mapfile -t libraries < <(find "$work/build" -type f -name liblinphone.so)
if [ "${#libraries[@]}" -ne 1 ]; then
    printf 'Expected one built SIP engine; found %s\n' "${#libraries[@]}" >&2
    exit 1
fi
"$tools/llvm-strip" --strip-unneeded -o "$work/engine/liblinphone.so" "${libraries[0]}"

upstream_version=5.6.0-alpha.190+026b4e9992
base=https://download.linphone.org/maven_repository/org/linphone/linphone-sdk-android
curl --fail --location --retry 3 --retry-all-errors -o "$work/upstream.aar" \
    "$base/$upstream_version/linphone-sdk-android-$upstream_version.aar"
python3 "$app_root/tools/check_android_sip_engine.py" --abi "$abi" --upstream "$work/upstream.aar" \
    --engine "$work/engine/liblinphone.so" --tools "$tools" --report "$work/engine/verification.json"
