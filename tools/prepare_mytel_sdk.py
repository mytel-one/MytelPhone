#!/usr/bin/env python3
"""Assemble the verified MyTel Android SIP SDK using the upstream Java API and resources."""
from __future__ import annotations

import argparse
import copy
import hashlib
import mmap
import os
from pathlib import Path
import shutil
import tempfile
from zipfile import ZipFile

from verify_android_distribution import DEFAULT_ABIS, inspect_elf

UPSTREAM_SHA256 = "595ba9d1e34623595c0fe69df230263af6dbcdd46ddea6f9c823868ba6b28eab"
PATCH_SHA256 = "18645df2437b1a422951b68eb369edb2d47eb690cdc18f77cd5c63b346a25ed7"
CONFIG = Path(__file__).resolve().parents[1] / "native-sdk"


def digest_file(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def read_pins() -> dict[str, str]:
    pins = {}
    for line in (CONFIG / "engines.sha256.properties").read_text(encoding="utf-8").splitlines():
        if line.strip() and not line.lstrip().startswith("#"):
            abi, digest = line.strip().split("=", 1)
            if abi in pins or abi not in DEFAULT_ABIS or len(bytes.fromhex(digest)) != 32:
                raise ValueError("Invalid native engine pin")
            pins[abi] = digest
    return pins


def prepare(upstream: Path, engines: Path, output: Path, abis: tuple[str, ...]) -> None:
    if output.exists():
        raise FileExistsError(output)
    if digest_file(upstream) != UPSTREAM_SHA256 or digest_file(CONFIG / "reliable-sip-return.patch") != PATCH_SHA256:
        raise ValueError("Unexpected upstream AAR or SDK source patch")
    pins = read_pins()
    for abi in abis:
        binary = engines / abi / "liblinphone.so"
        if digest_file(binary) != pins.get(abi):
            raise ValueError(f"Missing verified native engine or SHA-256 mismatch for {abi}")
        with binary.open("rb") as stream, mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
            inspect_elf(data, abi)

    fd, temporary = tempfile.mkstemp(dir=output.parent, suffix=".aar.tmp")
    os.close(fd)
    try:
        with ZipFile(upstream) as base, ZipFile(temporary, "w") as patched:
            replaced = set()
            for item in base.infolist():
                name = item.filename.split("/")
                replacement = None
                if len(name) >= 3 and name[0] == "jni":
                    abi = name[1]
                    if abi not in abis:
                        continue
                    if name[2:] == ["liblinphone.so"]:
                        if abi in replaced:
                            raise ValueError(f"Duplicate engine for {abi}")
                        replaced.add(abi)
                        with base.open(item) as old_engine:
                            if hashlib.file_digest(old_engine, "sha256").hexdigest() == pins[abi]:
                                raise ValueError(f"Upstream native engine incorrectly selected for {abi}")
                        replacement = engines / abi / "liblinphone.so"
                with (replacement.open("rb") if replacement else base.open(item)) as source:
                    with patched.open(copy.copy(item), "w") as destination:
                        shutil.copyfileobj(source, destination, 1024 * 1024)
            if replaced != set(abis):
                raise ValueError("The upstream SDK is missing a requested engine")
        # Publish only a closed, readable AAR with the expected engines.
        with ZipFile(temporary) as check:
            for abi in abis:
                with check.open(f"jni/{abi}/liblinphone.so") as stream:
                    if hashlib.file_digest(stream, "sha256").hexdigest() != pins[abi]:
                        raise ValueError(f"Packaged engine hash mismatch for {abi}")
        os.link(temporary, output)
    finally:
        Path(temporary).unlink(missing_ok=True)
    print(f"{digest_file(output)}  {output.name}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", required=True, type=Path)
    parser.add_argument("--engines", required=True, type=Path, help="Directory containing one liblinphone.so under each ABI")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--arm64-device-test", action="store_true", help="Prepare the single-ABI device test artifact")
    args = parser.parse_args()
    prepare(args.upstream, args.engines, args.output, ("arm64-v8a",) if args.arm64_device_test else DEFAULT_ABIS)
