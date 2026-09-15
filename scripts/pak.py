#!/usr/bin/env python3
"""Pack and unpack the PopCap PAK v0 format used by Plants vs. Zombies."""

import argparse
from pathlib import Path, PurePosixPath
import struct
import sys
import tempfile
from typing import Iterable


PAK_MAGIC = 0xBAC04AC0
PAK_VERSION = 0
PAK_XOR_KEY = 0xF7
PAK_END_FLAG = 0x80
MAX_FILE_SIZE = 0x7FFFFFFF
XOR_TABLE = bytes(aByte ^ PAK_XOR_KEY for aByte in range(256))


class PakFormatError(ValueError):
    """The PAK data cannot be parsed safely."""


def xor_data(theData: bytes) -> bytes:
    return theData.translate(XOR_TABLE)


def require_bytes(theData: bytes, thePosition: int, theSize: int, theDescription: str) -> bytes:
    if theSize < 0 or thePosition < 0 or thePosition + theSize > len(theData):
        raise PakFormatError(f"Unexpected end of PAK while reading {theDescription}.")
    return theData[thePosition:thePosition + theSize]


def safe_relative_path(theName: str) -> Path:
    aPath = PurePosixPath(theName.replace("\\", "/"))
    if not theName or aPath.is_absolute() or any(aPart in ("", ".", "..") for aPart in aPath.parts):
        raise PakFormatError(f"Unsafe PAK entry path: {theName!r}")
    if any(":" in aPart for aPart in aPath.parts):
        raise PakFormatError(f"Unsafe PAK entry path: {theName!r}")
    return Path(*aPath.parts)


def read_records(theData: bytes):
    if len(theData) < 9:
        raise PakFormatError("PAK is too small to contain a header.")

    aMagic, aVersion = struct.unpack_from("<II", theData)
    if aMagic != PAK_MAGIC:
        raise PakFormatError("Not a PopCap PAK v0 file (invalid magic).")
    if aVersion != PAK_VERSION:
        raise PakFormatError(f"Unsupported PAK version: {aVersion}.")

    aPosition = 8
    aRecords = []
    while True:
        aFlag = require_bytes(theData, aPosition, 1, "record flag")[0]
        aPosition += 1
        if aFlag & PAK_END_FLAG:
            break

        aNameLength = require_bytes(theData, aPosition, 1, "entry name length")[0]
        aPosition += 1
        aNameBytes = require_bytes(theData, aPosition, aNameLength, "entry name")
        aPosition += aNameLength
        try:
            aName = aNameBytes.decode("utf-8")
        except UnicodeDecodeError as anError:
            raise PakFormatError("PAK entry name is not valid UTF-8.") from anError

        aSize = struct.unpack("<i", require_bytes(theData, aPosition, 4, "entry size"))[0]
        aPosition += 4
        aFileTime = struct.unpack("<q", require_bytes(theData, aPosition, 8, "entry time"))[0]
        aPosition += 8
        if aSize < 0:
            raise PakFormatError(f"PAK entry has a negative size: {aName!r}")
        aRecords.append((safe_relative_path(aName), aSize, aFileTime))

    aDataPosition = aPosition
    for _, aSize, _ in aRecords:
        aDataPosition += aSize
        if aDataPosition > len(theData):
            raise PakFormatError("PAK entry data extends beyond the end of the file.")
    if aDataPosition != len(theData):
        raise PakFormatError("PAK contains trailing data after its file entries.")
    return aRecords, aPosition


def unpack(theInput: Path, theOutput: Path):
    aData = xor_data(theInput.read_bytes())
    aRecords, aDataPosition = read_records(aData)

    if theOutput.exists() and any(theOutput.iterdir()):
        raise PakFormatError(f"Refusing to unpack into non-empty directory: {theOutput}")
    theOutput.mkdir(parents=True, exist_ok=True)

    for aRelativePath, aSize, _ in aRecords:
        aDestination = theOutput / aRelativePath
        aDestination.parent.mkdir(parents=True, exist_ok=True)
        aDestination.write_bytes(aData[aDataPosition:aDataPosition + aSize])
        aDataPosition += aSize

    print(f"Unpacked {len(aRecords)} files to {theOutput}.")


def should_exclude(theRelativePath: Path, theExcludedPaths: set[PurePosixPath]) -> bool:
    aPath = PurePosixPath(theRelativePath.as_posix())
    return any(aPath == anExcluded or anExcluded in aPath.parents for anExcluded in theExcludedPaths)


def source_files(theSource: Path, theExcludedPaths: set[PurePosixPath]) -> Iterable[Path]:
    for aPath in sorted(theSource.rglob("*")):
        if aPath.is_file() and not should_exclude(aPath.relative_to(theSource), theExcludedPaths):
            yield aPath


def write_xored(theFile, theData: bytes):
    theFile.write(xor_data(theData))


def pack(theSource: Path, theOutput: Path, theExcludes: list[str]):
    if not theSource.is_dir():
        raise PakFormatError(f"Resource source directory does not exist: {theSource}")
    aSource = theSource.resolve()
    aOutput = theOutput.resolve()
    try:
        aOutput.relative_to(aSource)
    except ValueError:
        pass
    else:
        raise PakFormatError("Output PAK must not be placed inside the resource source directory.")

    aExcludedPaths = {PurePosixPath(anExclude.strip("/")) for anExclude in theExcludes}
    if any(not str(anExclude) or anExclude.is_absolute() or ".." in anExclude.parts
           for anExclude in aExcludedPaths):
        raise PakFormatError("Excluded paths must be relative paths inside the resource directory.")

    aFiles = list(source_files(aSource, aExcludedPaths))
    aRecords = []
    aHeaderSize = 8 + 1
    aTotalSize = aHeaderSize
    for aPath in aFiles:
        aRelativePath = aPath.relative_to(aSource)
        aName = aRelativePath.as_posix()
        aNameBytes = aName.encode("utf-8")
        aStat = aPath.stat()
        aSize = aStat.st_size
        if len(aNameBytes) > 0xFF:
            raise PakFormatError(f"PAK entry name exceeds 255 bytes: {aName}")
        if aSize > MAX_FILE_SIZE:
            raise PakFormatError(f"PAK entry exceeds 2 GiB: {aName}")
        aHeaderSize += 1 + 1 + len(aNameBytes) + 4 + 8
        aTotalSize += aSize
        if aHeaderSize > MAX_FILE_SIZE or aTotalSize > MAX_FILE_SIZE:
            raise PakFormatError("PAK exceeds the native reader's 2 GiB addressable range.")
        aRecords.append((aPath, aNameBytes, aSize, aStat.st_mtime_ns))

    theOutput.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("wb", dir=theOutput.parent, delete=False) as aTempFile:
        aTemporaryOutput = Path(aTempFile.name)
        try:
            write_xored(aTempFile, struct.pack("<II", PAK_MAGIC, PAK_VERSION))
            for _, aNameBytes, aSize, aFileTime in aRecords:
                write_xored(aTempFile, b"\0")
                write_xored(aTempFile, bytes((len(aNameBytes),)))
                write_xored(aTempFile, aNameBytes)
                write_xored(aTempFile, struct.pack("<iq", aSize, aFileTime))
            write_xored(aTempFile, bytes((PAK_END_FLAG,)))
            for aPath, _, _, _ in aRecords:
                with aPath.open("rb") as aSourceFile:
                    while aChunk := aSourceFile.read(1024 * 1024):
                        write_xored(aTempFile, aChunk)
            aTempFile.flush()
        except Exception:
            aTemporaryOutput.unlink(missing_ok=True)
            raise
    aTemporaryOutput.replace(theOutput)
    print(f"Packed {len(aRecords)} files into {theOutput}.")


def parse_args():
    aParser = argparse.ArgumentParser(description=__doc__)
    aCommands = aParser.add_subparsers(dest="command", required=True)

    aPackParser = aCommands.add_parser("pack", help="create a PAK from a resource directory")
    aPackParser.add_argument("--source", type=Path, required=True)
    aPackParser.add_argument("--output", type=Path, required=True)
    aPackParser.add_argument("--exclude", action="append", default=[],
                             help="top-level path to omit; may be specified more than once")

    anUnpackParser = aCommands.add_parser("unpack", help="extract a PAK into an empty directory")
    anUnpackParser.add_argument("--input", type=Path, required=True)
    anUnpackParser.add_argument("--output", type=Path, required=True)
    return aParser.parse_args()


def main():
    anArgs = parse_args()
    try:
        if anArgs.command == "pack":
            pack(anArgs.source, anArgs.output, anArgs.exclude)
        else:
            unpack(anArgs.input, anArgs.output)
    except (OSError, PakFormatError) as anError:
        print(f"Error: {anError}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
