#!/usr/bin/env python3
"""Verify APK/AAB native compatibility and official-tool metadata.
Exit codes: 0 PASS, 1 FAIL, 2 UNVERIFIED. Runtime acceptance is separate.
"""
from __future__ import annotations
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
import pathlib
import re
import shutil
import struct
import subprocess
import sys
import zipfile
import xml.etree.ElementTree as ET

DEFAULT_ABIS = ("armeabi-v7a", "arm64-v8a", "x86", "x86_64")
REQUIRED_LIBRARIES = ("liblinphone.so", "libmediastreamer2.so", "libbctoolbox.so", "libortp.so", "libc++_shared.so")
ABI_ELF = {"armeabi-v7a": (32, 40, 4096), "arm64-v8a": (64, 183, 16384), "x86": (32, 3, 4096), "x86_64": (64, 62, 16384)}
MAX_NATIVE_BYTES = 256 * 1024 * 1024
ANDROID_XML = "{http://schemas.android.com/apk/res/android}"


class VerificationError(ValueError):
    """Malformed data or a violated compatibility requirement."""


def inspect_elf(data: bytes, abi: str) -> dict:
    if abi not in ABI_ELF:
        raise VerificationError("Unsupported Android ABI: " + abi)
    if len(data) < 16 or data[:4] != b"\x7fELF":
        raise VerificationError("Invalid or truncated ELF identification")
    expected_bits, expected_machine, required_alignment = ABI_ELF[abi]
    bits = {1: 32, 2: 64}.get(data[4])
    if bits != expected_bits:
        raise VerificationError("ELF class does not match " + abi)
    if data[5] != 1 or data[6] != 1:
        raise VerificationError("Android libraries must use ELF version 1, little endian")
    header_size, program_size = (64, 56) if bits == 64 else (52, 32)
    if len(data) < header_size:
        raise VerificationError("Truncated ELF header")
    elf_type, machine, version = struct.unpack_from("<HHI", data, 16)
    if elf_type != 3 or machine != expected_machine or version != 1:
        raise VerificationError("ELF type, machine or version does not match " + abi)
    if bits == 64:
        program_offset = struct.unpack_from("<Q", data, 32)[0]
        actual_header_size, entry_size, count = struct.unpack_from("<HHH", data, 52)
    else:
        program_offset = struct.unpack_from("<I", data, 28)[0]
        actual_header_size, entry_size, count = struct.unpack_from("<HHH", data, 40)
    if actual_header_size != header_size or entry_size != program_size:
        raise VerificationError("Invalid ELF header or program-header entry size")
    if not count or count == 0xFFFF:
        raise VerificationError("Missing or unsupported extended program-header count")
    if program_offset < header_size or program_offset + count * entry_size > len(data):
        raise VerificationError("Program-header table is outside the file")
    loads = []
    for index in range(count):
        fields = struct.unpack_from("<IIQQQQQQ" if bits == 64 else "<IIIIIIII", data, program_offset + index * entry_size)
        if fields[0] != 1:
            continue
        if bits == 64:
            file_offset, virtual_address, file_size, memory_size, alignment = fields[2], fields[3], fields[5], fields[6], fields[7]
        else:
            file_offset, virtual_address, file_size, memory_size, alignment = fields[1], fields[2], fields[4], fields[5], fields[7]
        if file_size > memory_size or file_offset + file_size > len(data):
            raise VerificationError("PT_LOAD file range or memory size is invalid")
        if virtual_address + memory_size > (1 << bits):
            raise VerificationError("PT_LOAD virtual range overflows its ELF class")
        if alignment < required_alignment or alignment & (alignment - 1):
            raise VerificationError("PT_LOAD alignment is below the required page size or not a power of two")
        if (virtual_address - file_offset) % alignment:
            raise VerificationError("PT_LOAD virtual and file offsets are incongruent")
        loads.append({"file_offset": file_offset, "virtual_address": virtual_address, "file_size": file_size, "memory_size": memory_size, "alignment": alignment})
    if not loads:
        raise VerificationError("ELF has no PT_LOAD segments")
    return {"bits": bits, "machine": machine, "required_alignment": required_alignment, "minimum_load_alignment": min(x["alignment"] for x in loads), "load_segments": loads}


def _issue(code, message, entry=None):
    item = {"code": code, "message": message}
    if entry is not None:
        item["entry"] = entry
    return item


def _payload_offset(stream, entry):
    stream.seek(entry.header_offset)
    header = stream.read(30)
    if len(header) != 30 or header[:4] != b"PK\x03\x04":
        raise VerificationError("Invalid or truncated ZIP local header")
    if struct.unpack_from("<H", header, 8)[0] != entry.compress_type:
        raise VerificationError("Local and central ZIP compression methods differ")
    name_length, extra_length = struct.unpack_from("<HH", header, 26)
    payload = entry.header_offset + 30 + name_length + extra_length
    if payload + entry.compress_size > stream.seek(0, 2):
        raise VerificationError("ZIP native payload extends past the archive")
    return payload


def verify_native_archive(path, required_abis=DEFAULT_ABIS, required_libraries=REQUIRED_LIBRARIES):
    path = pathlib.Path(path)
    kind = "aab" if path.suffix.lower() == ".aab" else "apk"
    report = {"kind": kind, "abis": [], "libraries": {}, "native_libraries": [], "errors": [], "unverified": []}
    prefix = "base/lib/" if kind == "aab" else "lib/"
    try:
        with zipfile.ZipFile(path) as archive, path.open("rb") as raw:
            entries = archive.infolist()
            for name, count in Counter(x.filename for x in entries).items():
                if count > 1:
                    report["errors"].append(_issue("DUPLICATE_ENTRY", "Duplicate ZIP member", name))
            for entry in entries:
                if entry.filename.startswith("/") or ".." in entry.filename.split("/") or "\\" in entry.filename:
                    report["errors"].append(_issue("ARCHIVE_PATH_INVALID", "Unsafe ZIP member path", entry.filename))
                if kind == "aab" and not entry.filename.startswith(prefix) and re.match(r"^[^/]+/lib/[^/]+/[^/]+\.so$", entry.filename):
                    report["unverified"].append(_issue("NON_BASE_NATIVE_UNVERIFIED", "Native feature-module code requires a separate module check", entry.filename))
                if not entry.filename.startswith(prefix) or not entry.filename.endswith(".so"):
                    continue
                relative = entry.filename[len(prefix):].split("/")
                if len(relative) != 2:
                    report["errors"].append(_issue("LIB_PATH_INVALID", "Native library is not directly under its ABI", entry.filename))
                    continue
                abi, library = relative
                report["libraries"].setdefault(abi, []).append(library)
                if abi not in ABI_ELF:
                    report["errors"].append(_issue("ABI_UNSUPPORTED", "Unrecognized Android ABI", entry.filename))
                    continue
                if entry.flag_bits & 1 or entry.compress_type not in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED):
                    report["errors"].append(_issue("ZIP_NATIVE_FORMAT", "Encrypted or unsupported native compression", entry.filename))
                    continue
                if entry.file_size > MAX_NATIVE_BYTES:
                    report["errors"].append(_issue("NATIVE_TOO_LARGE", "Native member exceeds inspection size limit", entry.filename))
                    continue
                try:
                    data = archive.read(entry)  # Verifies this member's CRC.
                    info = inspect_elf(data, abi)
                    payload = _payload_offset(raw, entry)
                    info.update({"entry": entry.filename, "abi": abi, "library": library, "uncompressed_bytes": entry.file_size, "stored": entry.compress_type == zipfile.ZIP_STORED, "zip_payload_offset": payload})
                    report["native_libraries"].append(info)
                    if kind == "apk" and info["stored"] and payload % 16384:
                        report["errors"].append(_issue("SO_ALIGNMENT", "Stored APK .so payload is not aligned to 16 KB", entry.filename))
                except (VerificationError, zipfile.BadZipFile, RuntimeError, OSError, struct.error) as error:
                    report["errors"].append(_issue("ELF_INVALID", str(error), entry.filename))
            report["abis"] = sorted(report["libraries"])
            for abi in required_abis:
                if abi not in report["libraries"]:
                    report["errors"].append(_issue("ABI_MISSING", "Required ABI is absent: " + abi))
                    continue
                for library in required_libraries:
                    if library not in report["libraries"][abi]:
                        report["errors"].append(_issue("CORE_LIBRARY_MISSING", abi + " is missing " + library))
            for abi in report["libraries"]:
                report["libraries"][abi] = sorted(set(report["libraries"][abi]))
    except (zipfile.BadZipFile, OSError, ValueError) as error:
        report["errors"].append(_issue("ARCHIVE_INVALID", str(error)))
    return report


def parse_badging(text):
    patterns = {
        "package_name": r"^package:.*? name='([^']+)'",
        "version_code": r"^package:.*? versionCode='(\d+)'",
        "version_name": r"^package:.*? versionName='([^']*)'",
        "min_sdk": r"^sdkVersion:'(\d+)'",
        "target_sdk": r"^targetSdkVersion:'(\d+)'",
    }
    result = {}
    for field, pattern in patterns.items():
        match = re.search(pattern, text, re.MULTILINE)
        if not match:
            raise VerificationError("aapt did not provide complete numeric package/SDK/version metadata: " + field)
        result[field] = int(match.group(1)) if field in ("version_code", "min_sdk", "target_sdk") else match.group(1)
    native = re.search(r"^native-code:\s*(.*)$", text, re.MULTILINE)
    result["native_code"] = re.findall(r"'([^']+)'", native.group(1)) if native else []
    result["required_features"] = sorted(set(re.findall(r"^[ \t]*(?:uses-feature|uses-implied-feature):[ \t]*name='([^']+)'", text, re.MULTILINE)))
    permissions = set()
    for line in text.splitlines():
        if re.match(r"^[ \t]*uses-permission(?:-sdk-\d+)?:", line):
            permission = re.match(r"^[ \t]*uses-permission(?:-sdk-\d+)?:[ \t]*name='([^']+)'", line)
            if permission is None:
                raise VerificationError("aapt permission record does not contain a name")
            permissions.add(permission.group(1))
    result["permissions"] = sorted(permissions)
    return result


def parse_bundle_alignment(text):
    declared = set(re.findall(r"""(?:page_?alignment|alignment)["'\s]*:\s*["']?(PAGE_ALIGNMENT_[A-Z0-9_]+)""", text, re.IGNORECASE))
    if not declared:
        return None
    if len(declared) != 1:
        raise VerificationError("Bundle config contains conflicting page alignment declarations")
    return declared.pop().upper()


def parse_bundle_native_config(text):
    """Read the actual JSON native message; absence is not disabled/defaultfalse."""
    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise VerificationError("Bundle config contains duplicate JSON fields")
            result[key] = value
        return result
    try:
        config = json.loads(text, object_pairs_hook=unique)
        optimizations = config.get("optimizations", {})
        if not isinstance(optimizations, dict):
            raise VerificationError("Bundle optimizations must be an object")
        native = optimizations.get("uncompressNativeLibraries")
        if native is None and "uncompressNativeLibraries" not in optimizations:
            return {"native_config_present": False, "uncompress_native_libraries": None, "page_alignment": None}
        if not isinstance(native, dict) or set(native) - {"enabled", "alignment"}:
            raise VerificationError("Bundle native compression message is malformed/unsupported")
        enabled, alignment = native.get("enabled", False), native.get("alignment")
        if type(enabled) is not bool or alignment not in (None, "PAGE_ALIGNMENT_4K", "PAGE_ALIGNMENT_16K", "PAGE_ALIGNMENT_UNSPECIFIED"):
            raise VerificationError("Bundle native compression/alignment value is invalid")
        return {"native_config_present": True, "uncompress_native_libraries": enabled, "page_alignment": alignment}
    except (json.JSONDecodeError, AttributeError, TypeError) as error:
        raise VerificationError("Invalid bundletool config JSON") from error


def _boolean(text):
    value = re.sub(r"^\(type 0x12\)", "", text.strip(), flags=re.IGNORECASE).lower()
    if value in ("true", "false"):
        return value == "true"
    if re.fullmatch(r"0x[0-9a-f]+", value) and int(value, 16) in (0, 1, 0xFFFFFFFF):
        return int(value, 16) != 0
    raise VerificationError("extractNativeLibs is not an explicit supported boolean")


def parse_apk_extract_native_libs(text):
    lines = text.splitlines()
    applications = [(index, len(match[1].expandtabs())) for index, line in enumerate(lines) if (match := re.match(r"^([ \t]*)E: application\b", line))]
    if len(applications) != 1 or not re.search(r"(?m)^[ \t]*E: manifest\b", text):
        raise VerificationError("aapt XMLtree must contain one application")
    index, depth = applications[0]
    values = []
    for line in lines[index + 1:]:
        if re.match(r"^[ \t]*E:", line):
            break  # Application attributes precede its child elements.
        match = re.match(r"^([ \t]*)A: android:extractNativeLibs(?:\([^)]*\))?[ \t]*=[ \t]*(.*)$", line)
        if "A: android:extractNativeLibs" in line and match is None:
            raise VerificationError("Application extractNativeLibs attribute is malformed")
        if match:
            if len(match[1].expandtabs()) <= depth:
                raise VerificationError("extractNativeLibs is outside application attributes")
            values.append(_boolean(match[2]))
    if len(values) > 1:
        raise VerificationError("Application contains duplicate extractNativeLibs attributes")
    return values[0] if values else None


def native_packaging_policy(native, extract_native_libs, config=None):
    if native.get("kind") not in ("apk", "aab"):
        raise VerificationError("Unknown native packaging artifact kind")
    records = native.get("native_libraries", [])
    if not all(type(row.get("stored")) is bool for row in records):
        raise VerificationError("Native compression inventory is incomplete")
    compressed = sum(not row["stored"] for row in records)
    if native["kind"] == "aab":
        if not config or config.get("native_config_present") is not True:
            raise VerificationError("Bundle native compression message is absent/ambiguous")
        if config.get("uncompress_native_libraries") is False:
            if extract_native_libs is not True:
                raise VerificationError("Compressed bundle mode requires explicit extractNativeLibs=true")
            mode = "COMPRESSED_EXTRACTED"
        elif config.get("uncompress_native_libraries") is True and config.get("page_alignment") == "PAGE_ALIGNMENT_16K":
            mode = "UNCOMPRESSED_16K"
        else:
            raise VerificationError("Uncompressed bundle native libraries require PAGE_ALIGNMENT_16K")
    elif compressed:
        if extract_native_libs is not True:
            raise VerificationError("Compressed APK native libraries require explicit extractNativeLibs=true")
        mode = "COMPRESSED_EXTRACTED"
    else:
        mode = "STORED_16K"
    return {"mode": mode, "extract_native_libs": extract_native_libs, "native_members": len(records),
            "compressed_native_members": compressed, "stored_native_members": len(records) - compressed}


def _parse_bundle_manifest(text):
    try:
        root = ET.fromstring(text)
        sdk = root.find("uses-sdk")
        if sdk is None:
            raise VerificationError("Bundle manifest does not declare uses-sdk")
        permissions = set()
        for element in root:
            if re.fullmatch(r"uses-permission(?:-sdk-\d+)?", element.tag):
                permission = element.get(ANDROID_XML + "name")
                if not permission:
                    raise VerificationError("Bundle permission record does not contain android:name")
                permissions.add(permission)
        applications = root.findall("application")
        if len(applications) > 1:
            raise VerificationError("Bundle manifest contains multiple applications")
        extract = applications[0].get(ANDROID_XML + "extractNativeLibs") if applications else None
        if extract not in (None, "true", "false"):
            raise VerificationError("Bundle extractNativeLibs is not a boolean")
        return {
            "package_name": root.attrib["package"], "version_code": int(root.attrib[ANDROID_XML + "versionCode"]),
            "version_name": root.attrib[ANDROID_XML + "versionName"], "min_sdk": int(sdk.attrib[ANDROID_XML + "minSdkVersion"]),
            "target_sdk": int(sdk.attrib[ANDROID_XML + "targetSdkVersion"]),
            "required_features": sorted({x.get(ANDROID_XML + "name") for x in root.findall("uses-feature") if x.get(ANDROID_XML + "name") and x.get(ANDROID_XML + "required", "true") != "false"}),
            "permissions": sorted(permissions),
            "extract_native_libs": None if extract is None else extract == "true",
        }
    except (ET.ParseError, KeyError, ValueError) as error:
        raise VerificationError("Invalid or incomplete bundletool manifest: " + str(error)) from error


def _tool_command(tool, java, label):
    path = pathlib.Path(tool)
    if not path.is_file():
        raise VerificationError("Tool does not exist: " + str(path))
    if path.suffix.lower() in (".jar", ".bat", ".cmd"):
        jar = path if path.suffix.lower() == ".jar" else path.parent / "lib" / "apksigner.jar"
        if path.suffix.lower() != ".jar" and (label != "apksigner" or not jar.is_file()):
            raise VerificationError("Unsupported batch wrapper; provide its executable or JAR")
        executable = java or shutil.which("java")
        if not executable:
            raise VerificationError("Java executable was not provided and is not on PATH")
        return [executable, "-jar", str(jar)]
    return [str(path)]


def _run_tool(tool, arguments, label, options, report):
    if not tool:
        report["unverified"].append(_issue("TOOL_UNAVAILABLE", label + " was not supplied"))
        return None
    try:
        completed = subprocess.run(_tool_command(tool, options.java, label) + arguments, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=45, check=False, shell=False)
        report["tools"][label] = {"name": pathlib.Path(tool).name, "exit_code": completed.returncode}
        if completed.returncode:
            report["errors"].append(_issue("TOOL_FAILED", label + ": " + (completed.stderr or completed.stdout)[-3000:].strip()))
            return None
        return completed.stdout
    except (VerificationError, OSError, subprocess.TimeoutExpired) as error:
        report["errors"].append(_issue("TOOL_EXECUTION", label + ": " + str(error)))
        return None


def _compare_metadata(actual, expected, report):
    for field, value in expected.items():
        if actual.get(field) != value:
            report["errors"].append(_issue("METADATA_MISMATCH", field + ": expected " + repr(value) + ", found " + repr(actual.get(field))))


def _certificate_digest(text):
    return {re.sub(r"[^0-9a-fA-F]", "", item).lower() for item in re.findall(r"(?:certificate SHA-256 digest|SHA256):\s*([0-9a-fA-F:]{64,95})", text)}


def verify_distribution(options):
    path = pathlib.Path(options.artifact)
    native = verify_native_archive(path, options.abis, options.required_libraries)
    report = {"artifact": path.name, "checked_at_utc": datetime.now(timezone.utc).isoformat(), "errors": list(native.pop("errors")), "unverified": list(native.pop("unverified")), "native": native, "metadata": {}, "signature": {}, "tools": {}}
    if path.is_file():
        digest = hashlib.sha256()
        with path.open("rb") as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(block)
        report["artifact_sha256"] = digest.hexdigest()
    expected = {"package_name": options.package_name, "version_name": options.version_name, "version_code": options.version_code, "min_sdk": options.min_sdk, "target_sdk": options.target_sdk}
    if native["kind"] == "apk":
        badging = _run_tool(options.aapt, ["dump", "badging", str(path)], "aapt", options, report)
        if badging is not None:
            try:
                report["metadata"] = parse_badging(badging)
                _compare_metadata(report["metadata"], expected, report)
                if set(report["metadata"]["native_code"]) != set(native["abis"]):
                    report["errors"].append(_issue("ABI_METADATA_MISMATCH", "aapt native-code differs from ZIP ABI directories"))
            except VerificationError as error:
                report["errors"].append(_issue("METADATA_INVALID", str(error)))
        manifest = _run_tool(options.aapt, ["dump", "xmltree", str(path), "AndroidManifest.xml"], "aapt_manifest", options, report)
        if manifest is not None:
            try:
                extract = parse_apk_extract_native_libs(manifest)
                report["metadata"]["extract_native_libs"] = extract
                report["native"]["packaging"] = native_packaging_policy(native, extract)
            except VerificationError as error:
                report["errors"].append(_issue("NATIVE_PACKAGING", str(error)))
        _run_tool(options.zipalign, ["-c", "-P", "16", "-v", "4", str(path)], "zipalign", options, report)
        signing = _run_tool(options.apksigner, ["verify", "--verbose", "--print-certs", "--min-sdk-version", str(options.min_sdk), str(path)], "apksigner", options, report)
        if signing is not None:
            report["signature"]["cryptographic_verification"] = "PASS"
            report["signature"]["certificate_sha256"] = sorted(_certificate_digest(signing))
    else:
        manifest = _run_tool(options.bundletool, ["dump", "manifest", "--bundle=" + str(path), "--module=base"], "bundletool_manifest", options, report)
        if manifest is not None:
            try:
                report["metadata"] = _parse_bundle_manifest(manifest)
                _compare_metadata(report["metadata"], expected, report)
            except VerificationError as error:
                report["errors"].append(_issue("METADATA_INVALID", str(error)))
        config = _run_tool(options.bundletool, ["dump", "config", "--bundle=" + str(path)], "bundletool_config", options, report)
        if config is not None:
            try:
                packaging_config = parse_bundle_native_config(config)
                report["native"]["bundle_native_config"] = packaging_config
                report["native"]["bundle_page_alignment"] = packaging_config["page_alignment"]
                report["native"]["packaging"] = native_packaging_policy(native, report["metadata"].get("extract_native_libs"), packaging_config)
            except VerificationError as error:
                report["errors"].append(_issue("BUNDLE_ALIGNMENT", str(error)))
        signing = _run_tool(options.jarsigner, ["-J-Duser.language=en", "-J-Duser.country=US", "-verify", "-verbose", "-certs", str(path)], "jarsigner", options, report)
        if signing is not None:
            if "jar verified." not in signing.lower() or "jar is unsigned" in signing.lower():
                report["errors"].append(_issue("SIGNATURE_INVALID", "jarsigner did not confirm a verified signed bundle"))
            else:
                report["signature"]["cryptographic_verification"] = "PASS"
        certificate = _run_tool(options.keytool, ["-J-Duser.language=en", "-J-Duser.country=US", "-printcert", "-jarfile", str(path)], "keytool", options, report)
        if certificate is not None:
            report["signature"]["certificate_sha256"] = sorted(_certificate_digest(certificate))
    if options.certificate_sha256:
        actual = set(report["signature"].get("certificate_sha256", []))
        if not actual:
            report["unverified"].append(_issue("SIGNING_IDENTITY_UNVERIFIED", "No signing certificate digest was obtained"))
        elif actual != {options.certificate_sha256}:
            report["errors"].append(_issue("SIGNING_IDENTITY_MISMATCH", "Signing certificate set differs from the expected identity"))
        else:
            report["signature"]["identity_verification"] = "PASS"
    else:
        report["unverified"].append(_issue("SIGNING_IDENTITY_UNSPECIFIED", "Expected production certificate fingerprint was not supplied"))
    if report["metadata"]:
        for feature in options.optional_features:
            if feature in report["metadata"].get("required_features", []):
                report["errors"].append(_issue("HARDWARE_FILTER", feature + " must not be a required install feature"))
    report["status"] = "FAIL" if report["errors"] else "UNVERIFIED" if report["unverified"] else "PASS"
    report["scope"] = "Distribution structure, metadata and signature; runtime and real-device acceptance are separate."
    return report


def arguments(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifact", type=pathlib.Path)
    for option in ("package-name", "version-name"):
        parser.add_argument("--" + option, required=True)
    for option in ("version-code", "min-sdk", "target-sdk"):
        parser.add_argument("--" + option, type=int, required=True)
    parser.add_argument("--abis", default=",".join(DEFAULT_ABIS), help="Comma-separated required ABIs")
    parser.add_argument("--require-library", dest="required_libraries", action="append")
    parser.add_argument("--optional-feature", dest="optional_features", action="append", default=[])
    parser.add_argument("--certificate-sha256")
    for option in ("aapt", "apksigner", "zipalign", "bundletool", "jarsigner", "keytool", "java"):
        parser.add_argument("--" + option)
    parser.add_argument("--output", type=pathlib.Path)
    options = parser.parse_args(argv)
    options.abis = tuple(x.strip() for x in options.abis.split(",") if x.strip())
    if not options.abis or any(x not in ABI_ELF for x in options.abis):
        parser.error("--abis must contain supported Android ABIs")
    options.required_libraries = tuple(options.required_libraries or REQUIRED_LIBRARIES)
    if any("/" in name or "\\" in name or not name.endswith(".so") for name in options.required_libraries):
        parser.error("--require-library values must be .so basenames")
    if options.certificate_sha256:
        options.certificate_sha256 = options.certificate_sha256.replace(":", "").lower()
        if not re.fullmatch(r"[0-9a-f]{64}", options.certificate_sha256):
            parser.error("--certificate-sha256 must be a SHA-256 certificate fingerprint")
    return options


def main(argv=None):
    options = arguments(argv)
    report = verify_distribution(options)
    encoded = json.dumps(report, indent=2, ensure_ascii=False) + "\n"
    if options.output:
        options.output.parent.mkdir(parents=True, exist_ok=True)
        options.output.write_text(encoded, encoding="utf-8")
    sys.stdout.write(encoded)
    return {"PASS": 0, "FAIL": 1, "UNVERIFIED": 2}[report["status"]]


if __name__ == "__main__":
    raise SystemExit(main())
