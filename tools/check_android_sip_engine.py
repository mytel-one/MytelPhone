#!/usr/bin/env python3
"""Check a rebuilt Android SIP engine against the upstream SDK for the same architecture."""
from __future__ import annotations

import argparse
import hashlib
import json
import mmap
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from zipfile import ZipFile

from verify_android_distribution import DEFAULT_ABIS, inspect_elf


def sha256(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def symbol_sets(path: Path, nm: Path) -> tuple[set[str], set[str]]:
    result = subprocess.check_output(
        [nm, "--format=posix", "--dynamic", "--extern-only", path], encoding="utf-8"
    )
    exports, imports = set(), set()
    for line in result.splitlines():
        fields = line.split()
        if len(fields) < 2:
            continue
        name, kind = fields[:2]
        name = name.partition("@")[0]
        if kind == "U":
            imports.add(name)
        elif kind not in ("w", "v", "?"):
            exports.add(name)
    return exports, imports


def needed(path: Path, readelf: Path) -> set[str]:
    info = subprocess.check_output([readelf, "--dynamic", "--wide", path], encoding="utf-8")
    return set(re.findall(r"\(NEEDED\).*?\[([^\]]+)\]", info))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--abi", required=True, choices=DEFAULT_ABIS)
    parser.add_argument("--upstream", required=True, type=Path)
    parser.add_argument("--engine", required=True, type=Path)
    parser.add_argument("--tools", required=True, type=Path, help="NDK toolchain bin directory")
    parser.add_argument("--report", required=True, type=Path)
    args = parser.parse_args()

    prefix = f"jni/{args.abi}/"
    nm = args.tools / "llvm-nm"
    readelf = args.tools / "llvm-readelf"
    if sha256(args.upstream) != "595ba9d1e34623595c0fe69df230263af6dbcdd46ddea6f9c823868ba6b28eab":
        raise ValueError("Unexpected upstream SDK")
    with args.engine.open("rb") as handle, mmap.mmap(handle.fileno(), 0, access=mmap.ACCESS_READ) as view:
        elf = inspect_elf(view, args.abi)
    new_exports, new_imports = symbol_sets(args.engine, nm)
    new_deps = needed(args.engine, readelf)
    with tempfile.TemporaryDirectory() as tmpdir, ZipFile(args.upstream) as upstream:
        tmp = Path(tmpdir)
        for name in upstream.namelist():
            if name.startswith(prefix) and name.endswith(".so"):
                with upstream.open(name) as src, (tmp / Path(name).name).open("wb") as dst:
                    shutil.copyfileobj(src, dst, 1024 * 1024)
        stock = tmp / "liblinphone.so"
        if sha256(stock) == sha256(args.engine):
            raise ValueError("Unpatched upstream engine")
        old_exports, old_imports = symbol_sets(stock, nm)
        missing_exports = sorted({s for s in old_exports if s.startswith(("Java_", "linphone_"))} - new_exports)
        additional_deps = sorted(new_deps - needed(stock, readelf))
        if missing_exports or additional_deps:
            raise ValueError(f"Missing public exports: {missing_exports[:10]}; unexpected runtime dependencies: {additional_deps}")
        extra_imports = new_imports - old_imports
        triple = {
            "arm64-v8a": "aarch64-linux-android", "armeabi-v7a": "arm-linux-androideabi",
            "x86": "i686-linux-android", "x86_64": "x86_64-linux-android",
        }[args.abi]
        platform = args.tools.parent / "sysroot/usr/lib" / triple / "21"
        if extra_imports:
            providers = [lib for lib in tmp.glob("*.so") if lib != stock] + list(platform.glob("*.so"))
            for provider in providers:
                if not extra_imports:
                    break
                extra_imports.difference_update(symbol_sets(provider, nm)[0])
        if extra_imports:
            raise ValueError(f"Unresolved new external imports: {sorted(extra_imports)[:15]}")
    report = {
        "abi": args.abi, "engineSha256": sha256(args.engine), "nativeBits": elf["bits"],
        "minimumLoadAlignment": elf["minimum_load_alignment"],
        "oldJavaExports": sum(s.startswith("Java_") for s in old_exports),
        "newJavaExports": sum(s.startswith("Java_") for s in new_exports), "needed": sorted(new_deps),
        "missingPublicExports": [], "unresolvedAdditionalImports": [],
        "sourceCommit": "026b4e99928da77a75f7852dd5c8631bd1a3ae17",
        "sourcePatchSha256": "18645df2437b1a422951b68eb369edb2d47eb690cdc18f77cd5c63b346a25ed7",
    }
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"PASS: {args.abi} {report['engineSha256']}")


if __name__ == "__main__":
    main()
