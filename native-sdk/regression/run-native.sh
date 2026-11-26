#!/bin/bash
# Build both native controls from the pinned shipping SDK with system OpenSSL.
set -euo pipefail
if [[ $# != 2 ]]; then echo "usage: $0 SDK_GIT_REPOSITORY NEW_BUILD_DIRECTORY" >&2; exit 2; fi
repo=$(realpath "$1")
dest=$(realpath -m "$2")
here=$(cd "$(dirname "$0")" && pwd)
if [[ -e $dest ]]; then echo "Choose a new build directory." >&2; exit 2; fi
mkdir -p "$dest/src" "$dest/install"
git -C "$repo" -c core.autocrlf=false archive 026b4e9992 belle-sip bctoolbox belr | tar -xf - -C "$dest/src"
flags=(-G Ninja -DCMAKE_BUILD_TYPE=Release -DPROJECT_VERSION=5.6.0 -DCMAKE_INSTALL_PREFIX="$dest/install"
       -DCMAKE_PREFIX_PATH="$dest/install" -DENABLE_UNIT_TESTS=OFF -DENABLE_TESTS_COMPONENT=OFF -DENABLE_STRICT=ON -DBUILD_SHARED_LIBS=ON)
cmake -S "$dest/src/bctoolbox" -B "$dest/bct" "${flags[@]}" -DENABLE_MBEDTLS=OFF -DENABLE_OPENSSL=ON -DENABLE_DECAF=OFF
cmake --build "$dest/bct" --parallel 2 && cmake --install "$dest/bct"
cmake -S "$dest/src/belr" -B "$dest/belr" "${flags[@]}" -DENABLE_TOOLS=ON
cmake --build "$dest/belr" --parallel 2 && cmake --install "$dest/belr"
cmake -S "$dest/src/belle-sip" -B "$dest/belle" "${flags[@]}"
cmake --build "$dest/belle" --parallel 2 && cmake --install "$dest/belle"
mkdir "$dest/red"
cp -a "$dest/install/lib"/libbelle-sip.so* "$dest/red/"
(cd "$dest/src" && git apply --check "$here/../reliable-sip-return.patch" && git apply "$here/../reliable-sip-return.patch")
cmake --build "$dest/belle" --parallel 2 && cmake --install "$dest/belle"
cc -Wall -Wextra -Werror -I"$dest/install/include" -L"$dest/install/lib" "-Wl,-rpath,$dest/install/lib"   "$here/return-test.c" -lbelle-sip -o "$dest/return-test"
MYTEL_SIP_TEST_WORK="$dest" python3 -B "$here/return-test.py"
