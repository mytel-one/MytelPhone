# MyTel Android 3.1.1 verification

Local build, signed distribution, Bazaar signature reconstruction, API31 upgrade/regression and native API37/16KB checks passed. This record covers prepublication acceptance; release integrity and market approval are separate checks.

## Build identity and scope

Compiled source: `415e63eb0e7648edae08509aa56d550378d0e611`, clean before and after the full build. Package `one.mytel.softphone`, version3.1.1/code30101, minSdk24/Android7, targetSdk37; packaged ABIs: `armeabi-v7a`, `arm64-v8a`, `x86`, `x86_64`.

Unchanged production certificate SHA256: `60ebd14405aeb1fe58e277297182fb48cb77d07f8046982a07e494ba49b4d7f4`.

This Android-only maintenance release removes READ_PHONE_NUMBERS from the manifest and optional call-redirection request. Missing, empty, cancelled or denied phone-state permission results do not advance to the system-role request. The app requests READ_PHONE_STATE when enabling optional call redirection, which defaults off. Windows remains3.1.0.

## Verification results

| Gate | Result |
|---|---|
| Full Android build | PASS: debug unit tests, strict lint, release APK/R8 and AAB; exit0 |
| Android JUnit | 212 tests, 0 failures, 0 errors, 0 skipped |
| Strict lint | 0 errors, 173 warnings |
| Public distribution-validator tests | 41 passed |
| Permission-continuation policy | 8 passed after a meaningful failing baseline; part of Android JUnit, not an extra combined total |
| Direct APK and AAB | PASS: production signature/certificate, identity, SDK levels, four ABIs, merged permissions and native packaging |
| Bazaar AAB/BIN reconstruction | PASS: all 13 APK records, exact input binding, production certificate/v1+v2, manifests and native checks; strict universal report and shared receipt validation |
| Existing API31 upgrade | PASS: production install-r30100→30101, retained data and native call/chat regression |
| Native API37/16KB startup and call | PASS: production30101/min24/target37, actual16384-byte pages, extracted JNI startup and a connected local call |

Test-suite counts describe their individual scopes; no artificial combined test total is claimed. Lint warnings remain disclosed.

## Signed artifacts

| File | Bytes | SHA256 |
|---|---:|---|
| MyTel-v3.1.1-android.apk | 78,848,736 | `0e51c7b30e61af6b07811ff05449eacc5319a70d1d4198e575515892360054cd` |
| MyTel-v3.1.1-android.aab | 83,151,596 | `538f71eef19b7b54bfb92cedf98e15e5c3b36ecee733234cf758d2cf8800a2f3` |
| MyTel-v3.1.1-android.bin | 1,268,957 | `3d680342c9e47d9af1e00c6c8ff9d760840afd9fd95dded7f69f910380c6164d` |

The APK is below the previously displayed150MB upload threshold. For Bazaar's App Bundle workflow, the AAB and BIN above are a matched pair and must replace the previous version together.

## Compressed native packaging and Bazaar signatures

The pinned official bundle-signer0.1.14 embeds bundletool1.11, which aligns stored native-library ZIP entries to4096 bytes. This build uses AGP's supported compressed/extracted JNI packaging. See [Android packaging guidance](https://developer.android.com/guide/practices/page-sizes#update-packaging) and the [official signer release](https://github.com/cafebazaar/bundle-signer/releases/tag/v0.1.14).

Direct APK, AAB and reconstructed universal checks establish `COMPRESSED_EXTRACTED`, explicit `extractNativeLibs=true`,60 compressed native members and 0 stored native members. The AAB explicitly disables native uncompression. ELF alignment/congruence remains checked:64-bit PT_LOAD segments meet16KB requirements; valid32-bit libraries use their applicable alignment requirement. Compression does not waive native checks or establish lower RAM use.

Official signer JAR SHA256: `aef2c387f678558e3a4baddab9730fbdca64ef28a0e2e0245491e1088b4968f5`,32,101,447 bytes, source `047454dba5694c6e3924bbd7e1dc81928a7ac0cd`.

Keyless reconstruction matched all 13 BIN records one-to-one. Every reconstructed APK passed production-certificate, v1/v2 signature, manifest package/version-code, READ_PHONE_NUMBERS absence and applicable ELF/ZIP checks. Base/universal report3.1.1; configuration splits may omit versionName. The universal APK independently passed min24/target37/four-ABI and strict distribution verification. The production key/configuration were preserved; neither is included in public output.

Completed reconstruction receipt SHA256: `899c6a27a5b8686d1c856e4dd4928afbbb930cd63a187de5c1edfaf4c3340dc8`. Reconstructed universal compatibility-report SHA256: `7ede2569ab17f1be7f2a0f1536d53507de5454297e79c5036ee1a6017e58d16d`.

## Native API31 upgrade and localhost regression

The existing API31/x86_64 profile was upgraded using the exact signed production APK and install-r. Version30101/3.1.1, the same data inode/UID, single fictional account/auth record, both exact original Persian messages, two independent existing call-history outcomes, contact and Dark theme were verified after upgrade. No uninstall, data clear/reset, replacement userdata or data restoration was used.

The installed package no longer requests READ_PHONE_NUMBERS. Four core JNI libraries were actually loaded from this package's extracted library directory; this guest uses4096-byte pages.

Native Persian messages passed in both directions. INVITE count remained0 throughout the MESSAGE exchange: messaging did not ring the peer.

| Native call check | Actual interval evidence |
|---|---|
| Failed call recovery | Explicit486 rejection followed by a distinct successful dialog;5-second RTP increases RX249/TX219 |
| Hold/resume | Held and resumed SDP observed on the same dialog;5-second resumed RTP increases RX251/TX221 |
| Incoming call | Fresh incoming call answered in native UI; approximately5-second RTP increases RX250/TX161 |

These are Call-ID-bound interval packet deltas at the localhost fixture. RX counts packets received from the emulator; TX counts packets sent toward it. These counters do not measure physical speech quality or end-device playback. The dedicated VM/ADB session closed normally without forced termination or data reset.

An initial SystemUI ANR recovered through Wait before baseline capture; it was not a MyTel crash. The first incoming attempt expired while being observed and is excluded from the successful fresh incoming-call result.

Native receipt SHA256 bindings:

Aggregate API31 acceptance receipt: `7e307220e6e7cfb6e7f43a1bda091c11f87ee8432fbfb45d92c30d2941052466`.

| Receipt | SHA256 |
|---|---|
| Upgrade retention | `a32e8f0568a5b2df8258228945a3b937437bacc41c07bf26be4909a21558919a` |
| Two-way MESSAGE | `efe66d5ca2305255aaee7a74bedd17d07a0f43ba9aa635b032b020a04970e0ba` |
| New call after486 | `84b5d75a9656f52aada184973fa6bef3f247d551d8572f9f6f9cdba050132cab` |
| Hold/resume | `31c8e7d1e740584c457de449aa576ef2af80a3cc5f908e5ffc0e91cf372fb082` |
| Incoming call | `cce4c8851e3fc09e644d189721ab3a80ad505a0b5c680446ea128ead5f22ac23` |
| Normal closure | `2131a24967a76b75f85fcf9652f27d7b99113bb30461bf3265eca0d6065d213d` |

## Native API37 / Android17 / 16KB acceptance

The exact production3.1.1 APK was installed with install-r on the existing dedicated x86_64 profile. Actual API37, Android17,16384-byte pages, version30101/min24/target37 and the same data identity567004:10231:10231 were verified. The prior30100 profile was an earlier min32 candidate; it is not represented as the published final3.1.0 APK.

The installed package excludes READ_PHONE_NUMBERS. All four core libraries were loaded from this updated package's extracted native directory, with zero APK-direct native mappings. Native startup and a connected local call passed. A5-second interval on the same active dialog recorded fixture RX+250/TX+165, with no RTP socket resets. Native hangup produced BYE; the fixture, VM and owned ADB server then closed normally without data reset or forced termination.

A SystemUI wait dialog appeared during cold startup and resolved through Wait. Successful startup/call evidence was collected from the responsive application afterward.

Aggregate API37 acceptance receipt: `bffd5100fa7bfa1468acd26b5a5dd318839d641787bc98657dbdc0759810a3fb`.

| API37 receipt | SHA256 |
|---|---|
| Installed package/native mappings | `ec5c029d491325bc6903361dbdce46219ba0dbd125778d96ed9c19b22e29ea11` |
| Connected call interval | `7f150f777ac39894816ea7367a7e5c9065d6a396b2e84716b60c93b8e43e5baf` |
| Normal closure | `04f105b981be982999f260fab7765a22e9c627b393d5e558fc56a897ecc135f7` |

## Limits and publication scope

Artifact checks cover the four packaged ABIs; x86_64 emulator results do not establish every physical ARM handset, PBX, microphone/speaker quality, video, transfer/conference or Bluetooth path. The optional call-redirection UI was not natively replayed; its permission continuation is covered by the eight policy tests. No fresh complete Light-theme matrix is claimed. Prior3.1.0 evidence stays attached to its original builds. Unchanged SDK/corresponding-source and prebuilt-dependency limitations disclosed in the3.1.0 source manifests remain; a complete offline native SDK rebuild is not established.

Final GitHub source/archive sealing, tag/main and historical-asset preservation, publication and anonymous-download hashes are recorded separately after these build/runtime gates. Offline Bazaar verification does not establish store submission, review approval or server acceptance. Private credentials, profiles, recordings and native captures are excluded from public packages.
