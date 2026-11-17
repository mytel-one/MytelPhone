import importlib.util
import pathlib
import struct
import tempfile
import unittest
from unittest.mock import patch
import warnings
import zipfile

MODULE = pathlib.Path(__file__).resolve().parents[1] / "verify_android_distribution.py"
spec = importlib.util.spec_from_file_location("verify_android_distribution", MODULE)
verification = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verification)

ABIS = ("armeabi-v7a", "arm64-v8a", "x86", "x86_64")
CORE_LIBRARIES = ("liblinphone.so", "libmediastreamer2.so")

def elf(abi="arm64-v8a", alignment=None, vaddr=0, load_type=1):
    bits, machine = {"armeabi-v7a": (32, 40), "arm64-v8a": (64, 183), "x86": (32, 3), "x86_64": (64, 62)}[abi]
    alignment = alignment or (16384 if bits == 64 else 4096)
    data = bytearray(256)
    data[:16] = b"\x7fELF" + bytes([2 if bits == 64 else 1, 1, 1, 0]) + bytes(8)
    if bits == 64:
        struct.pack_into("<HHIQQQIHHHHHH", data, 16, 3, machine, 1, 0, 64, 0, 0, 64, 56, 1, 0, 0, 0)
        struct.pack_into("<IIQQQQQQ", data, 64, load_type, 5, 0, vaddr, vaddr, 256, 256, alignment)
    else:
        struct.pack_into("<HHIIIIIHHHHHH", data, 16, 3, machine, 1, 0, 52, 0, 0, 52, 32, 1, 0, 0, 0)
        struct.pack_into("<IIIIIIII", data, 52, load_type, 0, vaddr, vaddr, 256, 256, 5, alignment)
    return bytes(data)

def archive(path, kind="apk", omit_abi=None, omit_library=None, aligned=True, compressed=False, duplicate=False):
    prefix = "base/lib" if kind == "aab" else "lib"
    with zipfile.ZipFile(path, "w") as output:
        for abi in ABIS:
            if abi == omit_abi:
                continue
            for library in CORE_LIBRARIES:
                if (abi, library) == omit_library:
                    continue
                name = prefix + "/" + abi + "/" + library
                entry = zipfile.ZipInfo(name)
                entry.compress_type = zipfile.ZIP_DEFLATED if compressed else zipfile.ZIP_STORED
                if aligned and kind == "apk" and not compressed:
                    base = output.fp.tell() + 30 + len(name.encode("utf8"))
                    padding = (-base) % 16384
                    if padding and padding < 4:
                        padding += 16384
                    if padding:
                        entry.extra = struct.pack("<HH", 0xD935, padding - 4) + bytes(padding - 4)
                output.writestr(entry, elf(abi))
        if duplicate:
            with warnings.catch_warnings():
                warnings.simplefilter("ignore", UserWarning)
                output.writestr(prefix + "/x86/liblinphone.so", elf("x86"))

class ElfTests(unittest.TestCase):
    def test_every_supported_abi_accepts_its_correct_class_machine_and_alignment(self):
        for abi in ABIS:
            info = verification.inspect_elf(elf(abi), abi)
            self.assertEqual(64 if "64" in abi else 32, info["bits"])
            self.assertEqual(1, len(info["load_segments"]))

    def test_arm64_four_kilobyte_load_alignment_is_rejected(self):
        with self.assertRaises(verification.VerificationError):
            verification.inspect_elf(elf(alignment=4096), "arm64-v8a")

    def test_x86_64_incongruent_file_and_virtual_offsets_are_rejected(self):
        with self.assertRaises(verification.VerificationError):
            verification.inspect_elf(elf("x86_64", vaddr=4096), "x86_64")

    def test_four_kilobyte_loads_are_valid_for_thirty_two_bit_abis(self):
        self.assertEqual(32, verification.inspect_elf(elf("armeabi-v7a"), "armeabi-v7a")["bits"])
        self.assertEqual(32, verification.inspect_elf(elf("x86"), "x86")["bits"])

    def test_bad_magic_and_truncated_headers_are_rejected(self):
        for data in (b"not an elf", elf()[:40], elf()[:80]):
            with self.assertRaises(verification.VerificationError):
                verification.inspect_elf(data, "arm64-v8a")

    def test_wrong_machine_for_an_abi_is_rejected(self):
        with self.assertRaises(verification.VerificationError):
            verification.inspect_elf(elf("x86_64"), "arm64-v8a")

    def test_no_load_segments_is_rejected(self):
        with self.assertRaises(verification.VerificationError):
            verification.inspect_elf(elf(load_type=2), "arm64-v8a")

    def test_program_headers_outside_the_file_are_rejected(self):
        data = bytearray(elf())
        struct.pack_into("<Q", data, 32, 65536)
        with self.assertRaises(verification.VerificationError):
            verification.inspect_elf(bytes(data), "arm64-v8a")

    def test_load_file_range_outside_the_file_is_rejected(self):
        data = bytearray(elf())
        struct.pack_into("<Q", data, 64 + 32, 99999)
        struct.pack_into("<Q", data, 64 + 40, 99999)
        with self.assertRaises(verification.VerificationError):
            verification.inspect_elf(bytes(data), "arm64-v8a")

class NativeArchiveTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.path = pathlib.Path(self.temporary.name) / "test.apk"

    def verify(self):
        return verification.verify_native_archive(self.path, required_abis=ABIS, required_libraries=CORE_LIBRARIES)

    def test_complete_aligned_apk_passes_native_checks(self):
        archive(self.path)
        report = self.verify()
        self.assertEqual([], report["errors"])
        self.assertEqual(set(ABIS), set(report["abis"]))
        self.assertEqual(8, len(report["native_libraries"]))

    def test_missing_thirty_two_bit_abi_is_reported(self):
        archive(self.path, omit_abi="armeabi-v7a")
        self.assertIn("ABI_MISSING", [item["code"] for item in self.verify()["errors"]])

    def test_missing_core_jni_library_is_reported_per_abi(self):
        archive(self.path, omit_library=("x86", "liblinphone.so"))
        self.assertIn("CORE_LIBRARY_MISSING", [item["code"] for item in self.verify()["errors"]])

    def test_unaligned_stored_library_payload_is_rejected(self):
        archive(self.path, aligned=False)
        self.assertIn("SO_ALIGNMENT", [item["code"] for item in self.verify()["errors"]])

    def test_compressed_libraries_do_not_need_zip_payload_alignment(self):
        archive(self.path, aligned=False, compressed=True)
        self.assertEqual([], self.verify()["errors"])

    def test_aab_uses_base_native_paths_and_does_not_apply_apk_offsets(self):
        self.path = self.path.with_suffix(".aab")
        archive(self.path, kind="aab", aligned=False)
        report = self.verify()
        self.assertEqual("aab", report["kind"])
        self.assertEqual([], report["errors"])

    def test_duplicate_library_entries_are_rejected(self):
        archive(self.path, duplicate=True)
        self.assertIn("DUPLICATE_ENTRY", [item["code"] for item in self.verify()["errors"]])

    def test_corrupt_zip_is_an_explicit_failure(self):
        self.path.write_bytes(b"not a zip")
        self.assertIn("ARCHIVE_INVALID", [item["code"] for item in self.verify()["errors"]])

    def test_native_code_in_other_bundle_modules_is_explicitly_unverified(self):
        self.path = self.path.with_suffix(".aab")
        archive(self.path, kind="aab", aligned=False)
        with zipfile.ZipFile(self.path, "a") as output:
            output.writestr("feature/lib/x86_64/libextra.so", b"not inspected")
        self.assertIn("NON_BASE_NATIVE_UNVERIFIED", [item["code"] for item in self.verify()["unverified"]])

class MetadataTests(unittest.TestCase):
    def test_badging_extracts_explicit_metadata_and_required_features(self):
        text = """package: name='one.example.phone' versionCode='30100' versionName='3.1.0'
sdkVersion:'24'
targetSdkVersion:'37'
native-code: 'armeabi-v7a' 'arm64-v8a' 'x86' 'x86_64'
uses-feature: name='android.hardware.microphone'
uses-feature-not-required: name='android.hardware.camera'
"""
        metadata = verification.parse_badging(text)
        self.assertEqual("one.example.phone", metadata["package_name"])
        self.assertEqual(24, metadata["min_sdk"])
        self.assertEqual(30100, metadata["version_code"])
        self.assertEqual(["android.hardware.microphone"], metadata["required_features"])

    def test_missing_sdk_metadata_is_not_invented(self):
        with self.assertRaises(verification.VerificationError):
            verification.parse_badging("package: name='one.example.phone' versionCode='1' versionName='1.0'")

    def test_indented_aapt_feature_groups_keep_required_hardware_filters(self):
        text = """package: name='one.example.phone' versionCode='30100' versionName='3.1.0'
sdkVersion:'24'
targetSdkVersion:'37'
feature-group: label=''
  uses-feature: name='android.hardware.wifi'
  uses-implied-feature: name='android.hardware.bluetooth' reason='permission'
  uses-feature-not-required: name='android.hardware.camera'
"""
        self.assertEqual(
            ["android.hardware.bluetooth", "android.hardware.wifi"],
            verification.parse_badging(text)["required_features"],
        )

    def test_bundle_alignment_recognizes_declared_four_and_sixteen_kilobytes(self):
        self.assertEqual("PAGE_ALIGNMENT_16K", verification.parse_bundle_alignment("alignment: PAGE_ALIGNMENT_16K"))
        self.assertEqual("PAGE_ALIGNMENT_4K", verification.parse_bundle_alignment('{"alignment": "PAGE_ALIGNMENT_4K"}'))
        self.assertIsNone(verification.parse_bundle_alignment("optimizations {}"))


class PermissionMetadataTests(unittest.TestCase):
    BADGING = """package: name='one.example.phone' versionCode='30101' versionName='3.1.1'
sdkVersion:'24'
targetSdkVersion:'37'
"""
    MANIFEST = '<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="one.example.phone" android:versionCode="30101" android:versionName="3.1.1"><uses-sdk android:minSdkVersion="24" android:targetSdkVersion="37" />{}</manifest>'

    def test_badging_exposes_even_sdk_limited_phone_number_permission(self):
        text = self.BADGING + "uses-permission: name='android.permission.CAMERA'\nuses-permission: name='android.permission.READ_PHONE_NUMBERS' maxSdkVersion='30'\n"
        self.assertEqual(
            ["android.permission.CAMERA", "android.permission.READ_PHONE_NUMBERS"],
            verification.parse_badging(text)["permissions"],
        )

    def test_badging_permission_inventory_is_sorted_unique_and_handles_sdk_records(self):
        text = self.BADGING + "  uses-permission-sdk-23: name='android.permission.RECORD_AUDIO'\nuses-permission: name='android.permission.CAMERA'\nuses-permission: name='android.permission.CAMERA'\npermission: name='one.example.permission.DECLARED'\n"
        self.assertEqual(
            ["android.permission.CAMERA", "android.permission.RECORD_AUDIO"],
            verification.parse_badging(text)["permissions"],
        )

    def test_badging_malformed_permission_record_is_not_silently_omitted(self):
        with self.assertRaises(verification.VerificationError):
            verification.parse_badging(self.BADGING + "uses-permission: maxSdkVersion='30'\n")

    def test_bundle_exposes_sdk_limited_permission_and_ignores_custom_declarations(self):
        body = '<uses-permission android:name="android.permission.CAMERA" /><uses-permission-sdk-23 android:name="android.permission.READ_PHONE_NUMBERS" android:maxSdkVersion="30" /><permission android:name="one.example.permission.DECLARED" />'
        self.assertEqual(
            ["android.permission.CAMERA", "android.permission.READ_PHONE_NUMBERS"],
            verification._parse_bundle_manifest(self.MANIFEST.format(body))["permissions"],
        )

    def test_bundle_permission_inventory_is_sorted_unique_and_top_level_only(self):
        body = '<uses-permission android:name="android.permission.RECORD_AUDIO" /><uses-permission android:name="android.permission.CAMERA" /><uses-permission android:name="android.permission.CAMERA" /><application><meta-data android:name="android.permission.READ_PHONE_NUMBERS" /></application>'
        self.assertEqual(
            ["android.permission.CAMERA", "android.permission.RECORD_AUDIO"],
            verification._parse_bundle_manifest(self.MANIFEST.format(body))["permissions"],
        )

    def test_bundle_malformed_permission_record_is_not_silently_omitted(self):
        with self.assertRaises(verification.VerificationError):
            verification._parse_bundle_manifest(self.MANIFEST.format('<uses-permission android:maxSdkVersion="30" />'))

    def test_an_empty_inventory_is_explicit_for_both_artifact_formats(self):
        self.assertEqual([], verification.parse_badging(self.BADGING)["permissions"])
        self.assertEqual([], verification._parse_bundle_manifest(self.MANIFEST.format(""))["permissions"])


class DistributionReportTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.path = pathlib.Path(self.temporary.name) / "test.apk"
        archive(self.path)

    def options(self, *extra):
        return verification.arguments([
            str(self.path), "--package-name", "one.example.phone",
            "--version-name", "3.1.0", "--version-code", "30100",
            "--min-sdk", "24", "--target-sdk", "37",
            "--require-library", "liblinphone.so", "--require-library", "libmediastreamer2.so",
            *extra,
        ])

    @staticmethod
    def official_output(tool, arguments, label, options, report):
        return {
            "aapt": "package: name='one.example.phone' versionCode='30100' versionName='3.1.0'\nsdkVersion:'24'\ntargetSdkVersion:'37'\nnative-code: 'armeabi-v7a' 'arm64-v8a' 'x86' 'x86_64'\n",
            "aapt_manifest": "E: manifest (line=1)\n  E: application (line=2)\n    A: android:extractNativeLibs(0x010104ea)=(type 0x12)0x0\n",
            "zipalign": "Verification successful",
            "apksigner": "Signer #1 certificate SHA-256 digest: " + "a" * 64,
            "bundletool_manifest": '<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="one.example.phone" android:versionCode="30100" android:versionName="3.1.0"><uses-sdk android:minSdkVersion="24" android:targetSdkVersion="37" /><application android:extractNativeLibs="false" /></manifest>',
            "bundletool_config": '{"optimizations":{"uncompressNativeLibraries":{"enabled":true,"alignment":"PAGE_ALIGNMENT_16K"}}}',
            "jarsigner": "jar verified.",
            "keytool": "SHA256: " + "a" * 64,
        }[label]

    def test_missing_official_tools_report_unverified_instead_of_pass(self):
        report = verification.verify_distribution(self.options())
        self.assertEqual("UNVERIFIED", report["status"])
        self.assertEqual([], report["errors"])
        self.assertIn("TOOL_UNAVAILABLE", [item["code"] for item in report["unverified"]])

    def test_matching_official_metadata_and_expected_identity_complete_the_gate(self):
        with patch.object(verification, "_run_tool", side_effect=self.official_output):
            report = verification.verify_distribution(self.options("--certificate-sha256", "a" * 64))
        self.assertEqual("PASS", report["status"])
        self.assertEqual([], report["unverified"])

    def test_metadata_mismatch_is_a_failure(self):
        options = self.options("--certificate-sha256", "a" * 64)
        options.version_code += 1
        with patch.object(verification, "_run_tool", side_effect=self.official_output):
            report = verification.verify_distribution(options)
        self.assertEqual("FAIL", report["status"])
        self.assertIn("METADATA_MISMATCH", [item["code"] for item in report["errors"]])

    def test_other_signing_identity_is_a_failure(self):
        with patch.object(verification, "_run_tool", side_effect=self.official_output):
            report = verification.verify_distribution(self.options("--certificate-sha256", "b" * 64))
        self.assertEqual("FAIL", report["status"])
        self.assertIn("SIGNING_IDENTITY_MISMATCH", [item["code"] for item in report["errors"]])

    def test_explicit_bundle_four_kilobyte_alignment_is_rejected(self):
        self.path = self.path.with_suffix(".aab")
        archive(self.path, kind="aab", aligned=False)
        def output(tool, arguments, label, options, report):
            if label == "bundletool_config":
                return '{"optimizations":{"uncompressNativeLibraries":{"enabled":true,"alignment":"PAGE_ALIGNMENT_4K"}}}'
            return self.official_output(tool, arguments, label, options, report)
        with patch.object(verification, "_run_tool", side_effect=output):
            report = verification.verify_distribution(self.options("--certificate-sha256", "a" * 64))
        self.assertEqual("FAIL", report["status"])
        self.assertIn("BUNDLE_ALIGNMENT", [item["code"] for item in report["errors"]])

    def test_compressed_apk_requires_actual_manifest_extraction_true(self):
        archive(self.path, compressed=True)
        for value, expected in (("0xffffffff", "PASS"), ("0x0", "FAIL")):
            def output(tool, arguments, label, options, report):
                if label == "aapt_manifest":
                    return "E: manifest (line=1)\n  E: application (line=2)\n    A: android:extractNativeLibs(0x010104ea)=(type 0x12)" + value + "\n"
                return self.official_output(tool, arguments, label, options, report)
            with self.subTest(value=value), patch.object(verification, "_run_tool", side_effect=output):
                report = verification.verify_distribution(self.options("--certificate-sha256", "a" * 64))
                self.assertEqual(expected, report["status"])

    def test_compressed_bundle_accepts_disabled_native_uncompression_with_4k_config(self):
        self.path = self.path.with_suffix(".aab")
        archive(self.path, kind="aab", compressed=True)
        def output(tool, arguments, label, options, report):
            value = self.official_output(tool, arguments, label, options, report)
            if label == "bundletool_config":
                return '{"optimizations":{"uncompressNativeLibraries":{"enabled":false,"alignment":"PAGE_ALIGNMENT_4K"}}}'
            return value.replace('extractNativeLibs="false"', 'extractNativeLibs="true"') if label == "bundletool_manifest" else value
        with patch.object(verification, "_run_tool", side_effect=output):
            report = verification.verify_distribution(self.options("--certificate-sha256", "a" * 64))
        self.assertEqual("PASS", report["status"])
        self.assertEqual("COMPRESSED_EXTRACTED", report["native"]["packaging"]["mode"])


class PackagingMetadataTests(unittest.TestCase):
    def test_bundle_native_message_presence_distinguishes_false_default_from_absence(self):
        for body in ('{}', '{"enabled":false}', '{"enabled":false,"alignment":"PAGE_ALIGNMENT_4K"}'):
            config = verification.parse_bundle_native_config('{"optimizations":{"uncompressNativeLibraries":' + body + '}}')
            self.assertIs(config["uncompress_native_libraries"], False)
            self.assertTrue(config["native_config_present"])
        config = verification.parse_bundle_native_config('{"optimizations":{}}')
        self.assertIsNone(config["uncompress_native_libraries"])
        self.assertFalse(config["native_config_present"])

    def test_bundle_config_rejects_malformed_bool_duplicate_keys_or_native_message(self):
        for text in ('{"optimizations":{"uncompressNativeLibraries":{"enabled":"false"}}}',
                     '{"optimizations":{"uncompressNativeLibraries":[]}}',
                     '{"optimizations":{"uncompressNativeLibraries":{"enabled":true,"enabled":false}}}',
                     '{"optimizations":{"uncompressNativeLibraries":{"alignment":"UNKNOWN"}}}', 'not JSON'):
            with self.subTest(text=text), self.assertRaises(verification.VerificationError):
                verification.parse_bundle_native_config(text)

    def test_apk_xmltree_reads_only_direct_application_boolean(self):
        for value, expected in (("0xffffffff", True), ("0x0", False), ("true", True), ("false", False)):
            text = "E: manifest\n  E: application\n    A: android:extractNativeLibs(0x010104ea)=(type 0x12)" + value + "\n    E: activity\n      A: android:extractNativeLibs(0x010104ea)=(type 0x12)0x0\n"
            self.assertIs(verification.parse_apk_extract_native_libs(text), expected)
        self.assertIsNone(verification.parse_apk_extract_native_libs("E: manifest\n  E: application\n    E: activity\n      A: android:extractNativeLibs(0x010104ea)=(type 0x12)0xffffffff\n"))

    def test_apk_xmltree_rejects_missing_application_duplicate_or_invalid_boolean(self):
        for text in ("E: manifest", "E: manifest\n  E: application\n  E: application", "E: manifest\n  E: application\n    A: android:extractNativeLibs(0x010104ea)=(type 0x12)0x2", "E: manifest\n  E: application\n    A: android:extractNativeLibs(0x010104ea)true", "E: manifest\n  E: application\n    A: android:extractNativeLibs(0x010104ea)=true\n    A: android:extractNativeLibs(0x010104ea)=false"):
            with self.subTest(text=text), self.assertRaises(verification.VerificationError):
                verification.parse_apk_extract_native_libs(text)

    def test_compressed_policy_rejects_ambiguous_or_unsafe_flags(self):
        native = {"kind": "aab", "native_libraries": [{"stored": False}]}
        for config in ({"native_config_present": False, "uncompress_native_libraries": None, "page_alignment": None},
                       {"native_config_present": True, "uncompress_native_libraries": True, "page_alignment": "PAGE_ALIGNMENT_4K"}):
            with self.subTest(config=config), self.assertRaises(verification.VerificationError):
                verification.native_packaging_policy(native, True, config)
        disabled = {"native_config_present": True, "uncompress_native_libraries": False, "page_alignment": None}
        for extract in (False, None):
            with self.subTest(extract=extract), self.assertRaises(verification.VerificationError):
                verification.native_packaging_policy(native, extract, disabled)

if __name__ == "__main__":
    unittest.main()

