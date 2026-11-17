# MyTel Android 3.1.1 verification

Status: validation in progress; this record must be completed before publication.

## Scope

Android-only maintenance release following the separately verified 3.1.0 release. Remove READ_PHONE_NUMBERS from the manifest and optional call-redirection runtime request, preserve other contact/media features, and handle cancelled/denied phone-state permission results without requesting the system role. The Windows release remains 3.1.0; its artifacts and historical tags are unchanged.

The production Android package/signing identity, minSdk24, targetSdk37 and four-ABI install contract remain unchanged. VersionName3.1.1/versionCode30101 prevent same-version update ambiguity. APK/AAB metadata checks must inspect the actual merged permissions rather than infer absence from source.

## Focused checks

- Permission continuation: baseline6/8 failures, corrected8/8 passed. Covers required grant, denial, cancellation/empty result, unrelated legacy permission and late results after activity finish.
- Artifact-permission metadata: seven added parser cases fail on the baseline; all34 validator tests pass after correction. APK badging and bundletool XML include SDK-limited permissions and fail on malformed records.
- Full Android unit/lint/release-R8 build, final signature/distribution checks, native upgrade/permission regression and matching CafeBazaar BIN reconstruction remain pending.

## Distribution requirements

The universal APK contains all four ABIs and may exceed CafeBazaar's displayed150MB upload limit. Store upload uses the smaller AAB and its matching offline-generated BIN. A BIN belongs to the exact verified AAB; mixing versions is unsupported. The existing production signing key remains private and unchanged.

The official CafeBazaar bundle-signer tool must be pinned by release/hash and its reconstructed APKs independently verified for signatures, production certificate and native packaging. The tool's old bundled bundletool does not establish target37/16KB compatibility by assumption.

## Limits

Prior3.1.0 runtime evidence remains tied to its original builds. This narrow patch requires its own listed acceptance; it does not claim every phone/PBX, physical speech-quality or untested video/transfer/Bluetooth behavior. Native SDK sources/dependency gaps remain disclosed in the3.1.0 source manifests. Store review/submission is separate from local builds or GitHub publication.
