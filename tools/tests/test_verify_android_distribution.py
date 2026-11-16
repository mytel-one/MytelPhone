import importlib.util
import pathlib
import struct
import tempfile
import unittest
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

    def test_bundle_alignment_recognizes_declared_four_and_sixteen_kilobytes(self):
        self.assertEqual("PAGE_ALIGNMENT_16K", verification.parse_bundle_alignment("alignment: PAGE_ALIGNMENT_16K"))
        self.assertEqual("PAGE_ALIGNMENT_4K", verification.parse_bundle_alignment('{"alignment": "PAGE_ALIGNMENT_4K"}'))
        self.assertIsNone(verification.parse_bundle_alignment("optimizations {}"))

if __name__ == "__main__":
    unittest.main()

