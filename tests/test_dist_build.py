#!/usr/bin/env python3
"""Integration test for the default native distribution target."""

from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = PROJECT_ROOT / "build"
DIST_DIR = PROJECT_ROOT / "dist"
sys.path.insert(0, str(Path(__file__).resolve().parent))

from test_pak_tool import read_pak


class DistBuildTest(unittest.TestCase):
    def test_cmake_rejects_distribution_directory_inside_resources(self):
        """An unsafe dist override must not be able to delete source properties."""
        with tempfile.TemporaryDirectory() as aTempDir:
            aBuildDirectory = Path(aTempDir) / "build"
            aResult = subprocess.run(
                ["cmake", "-G", "Ninja", "-S", str(PROJECT_ROOT), "-B", str(aBuildDirectory),
                 f"-DPVZ_DIST_DIR={PROJECT_ROOT / 'res'}"],
                capture_output=True,
                text=True,
            )
        self.assertNotEqual(aResult.returncode, 0)
        self.assertIn("must not overlap", aResult.stdout + aResult.stderr)

    def test_cmake_rejects_distribution_symlinked_to_resources(self):
        """A symlinked dist override must not bypass the source-directory guard."""
        with tempfile.TemporaryDirectory() as aTempDir:
            aTemporaryPath = Path(aTempDir)
            aDistributionLink = aTemporaryPath / "resource-link"
            os.symlink(PROJECT_ROOT / "res", aDistributionLink)
            aResult = subprocess.run(
                ["cmake", "-G", "Ninja", "-S", str(PROJECT_ROOT),
                 "-B", str(aTemporaryPath / "build"),
                 f"-DPVZ_DIST_DIR={aDistributionLink}"],
                capture_output=True,
                text=True,
            )
        self.assertNotEqual(aResult.returncode, 0)
        self.assertIn("must not overlap", aResult.stdout + aResult.stderr)

    def test_fresh_configuration_stages_resources(self):
        """A clean CMake configuration must package main resources and external properties."""
        with tempfile.TemporaryDirectory() as aTempDir:
            aTemporaryPath = Path(aTempDir)
            aBuildDirectory = aTemporaryPath / "build"
            aDistributionDirectory = aTemporaryPath / "dist"
            aConfigureResult = subprocess.run(
                ["cmake", "-G", "Ninja", "-S", str(PROJECT_ROOT), "-B", str(aBuildDirectory),
                 f"-DPVZ_DIST_DIR={aDistributionDirectory}"],
                capture_output=True,
                text=True,
            )
            self.assertEqual(aConfigureResult.returncode, 0,
                             aConfigureResult.stdout + aConfigureResult.stderr)
            aBuildResult = subprocess.run(
                ["cmake", "--build", str(aBuildDirectory), "--target", "pvz-resources"],
                capture_output=True,
                text=True,
            )
            self.assertEqual(aBuildResult.returncode, 0, aBuildResult.stdout + aBuildResult.stderr)
            self.assertTrue((aDistributionDirectory / "main.pak").is_file())
            self.assertTrue((aDistributionDirectory / "properties" / "default.xml").is_file())
            _, _, aPakFiles = read_pak(aDistributionDirectory / "main.pak")
            self.assertTrue("images/Almanac.png" in aPakFiles)
            self.assertTrue("properties/LawnStrings.txt" in aPakFiles)

    def test_dist_target_stages_executable_pak_and_properties(self):
        """A release build regression must not omit a runtime resource component."""
        aResult = subprocess.run(
            ["cmake", "--build", str(BUILD_DIR), "--target", "pvz-dist"],
            capture_output=True,
            text=True,
        )
        self.assertEqual(aResult.returncode, 0, aResult.stdout + aResult.stderr)
        self.assertTrue((DIST_DIR / "main.pak").is_file())
        self.assertTrue((DIST_DIR / "properties" / "default.xml").is_file())
        self.assertTrue(any(aPath.is_file() and aPath.name.startswith("pvz-portable")
                            for aPath in DIST_DIR.iterdir()))
        _, _, aPakFiles = read_pak(DIST_DIR / "main.pak")
        self.assertTrue("images/Almanac.png" in aPakFiles, "main.pak is missing images/Almanac.png")
        self.assertTrue("properties/LawnStrings.txt" in aPakFiles,
                        "main.pak is missing properties/LawnStrings.txt")
        self.assertFalse(any(aName.startswith("main/") for aName in aPakFiles))


if __name__ == "__main__":
    unittest.main()
