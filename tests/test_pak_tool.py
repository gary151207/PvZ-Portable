#!/usr/bin/env python3
"""Behavior tests for the PopCap PAK build tool."""

import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]
PAK_TOOL = PROJECT_ROOT / "scripts" / "pak.py"


def read_pak(thePath: Path):
    """Independently decode the small PAK v0 container used by the game."""
    aData = bytearray(thePath.read_bytes())
    for anIndex in range(len(aData)):
        aData[anIndex] ^= 0xF7

    aMagic, aVersion = struct.unpack_from("<II", aData)
    aPosition = 8
    aRecords = []
    while not (aData[aPosition] & 0x80):
        aPosition += 1
        aNameSize = aData[aPosition]
        aPosition += 1
        aName = bytes(aData[aPosition:aPosition + aNameSize]).decode("utf-8")
        aPosition += aNameSize
        aSize = struct.unpack_from("<i", aData, aPosition)[0]
        aPosition += 4
        aPosition += 8  # File time is metadata and not part of the content contract.
        aRecords.append((aName, aSize))
    aPosition += 1

    aFiles = {}
    for aName, aSize in aRecords:
        aFiles[aName] = bytes(aData[aPosition:aPosition + aSize])
        aPosition += aSize
    return aMagic, aVersion, aFiles


class PakToolTest(unittest.TestCase):
    def run_tool(self, *theArgs: str):
        return subprocess.run(
            [sys.executable, str(PAK_TOOL), *theArgs],
            capture_output=True,
            text=True,
        )

    def test_pack_honors_an_optional_excluded_directory(self):
        """A packer regression must not emit a caller-excluded resource subtree."""
        with tempfile.TemporaryDirectory() as aTempDir:
            aRoot = Path(aTempDir)
            aSource = aRoot / "res"
            (aSource / "images").mkdir(parents=True)
            (aSource / "properties").mkdir()
            (aSource / "alpha.txt").write_bytes(b"alpha\n")
            (aSource / "images" / "leaf.bin").write_bytes(b"\x00\x10\xff")
            (aSource / "properties" / "default.xml").write_text("<Properties />\n")
            os.utime(aSource / "alpha.txt", (1_700_000_000, 1_700_000_000))

            aPak = aRoot / "main.pak"
            aResult = self.run_tool("pack", "--source", str(aSource), "--output", str(aPak),
                                    "--exclude", "properties")
            self.assertEqual(aResult.returncode, 0, aResult.stderr)

            aMagic, aVersion, aFiles = read_pak(aPak)
            self.assertEqual(aMagic, 0xBAC04AC0)
            self.assertEqual(aVersion, 0)
            self.assertEqual(aFiles, {
                "alpha.txt": b"alpha\n",
                "images/leaf.bin": b"\x00\x10\xff",
            })

    def test_unpack_expands_a_known_popcap_pak(self):
        """A decoder regression must not corrupt an independently authored PAK fixture."""
        # Decoded fixture: v0 header, HELLO.TXT (2 bytes, timestamp 0), end flag, "ok".
        aReferencePak = bytes.fromhex(
            "37bd374df7f7f7f7f7febfb2bbbbb8d9a3afa3f5f7f7f7f7f7f7f7f7f7f7f777989c"
        )
        with tempfile.TemporaryDirectory() as aTempDir:
            aRoot = Path(aTempDir)
            aPak = aRoot / "reference.pak"
            aPak.write_bytes(aReferencePak)
            aOutput = aRoot / "unpacked"

            aResult = self.run_tool("unpack", "--input", str(aPak), "--output", str(aOutput))
            self.assertEqual(aResult.returncode, 0, aResult.stderr)

            self.assertEqual((aOutput / "HELLO.TXT").read_bytes(), b"ok")


if __name__ == "__main__":
    unittest.main()
