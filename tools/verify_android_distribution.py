"""Verify Android distribution metadata and native compatibility."""


class VerificationError(ValueError):
    pass


def inspect_elf(data, abi):
    raise NotImplementedError("ELF compatibility validation has not been implemented")


def verify_native_archive(path, required_abis, required_libraries):
    raise NotImplementedError("Native archive validation has not been implemented")


def parse_badging(text):
    raise NotImplementedError("Official aapt metadata parsing has not been implemented")


def parse_bundle_alignment(text):
    raise NotImplementedError("Bundle alignment inspection has not been implemented")
