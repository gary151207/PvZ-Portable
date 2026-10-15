"""Fetch package files from PyPI without pip's sdist build machinery.

Usage:  python pypi_get.py <package> [<package> ...]
Downloads every wheel matching this interpreter (or the best sdist) into .wheels.
"""
from __future__ import annotations

import json
import os
import ssl
import sys
import urllib.request

WS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(WS, ".wheels")
CTX = ssl.create_default_context()


def get_json(url: str) -> dict:
    req = urllib.request.Request(url, headers={"User-Agent": "dsh-font-extract/1.0"})
    with urllib.request.urlopen(req, timeout=120, context=CTX) as resp:
        return json.load(resp)


def download(url: str, dest: str) -> None:
    req = urllib.request.Request(url, headers={"User-Agent": "dsh-font-extract/1.0"})
    with urllib.request.urlopen(req, timeout=600, context=CTX) as resp, open(dest, "wb") as fh:
        while True:
            chunk = resp.read(1 << 20)
            if not chunk:
                break
            fh.write(chunk)


def main(pkgs: list[str]) -> int:
    os.makedirs(OUT, exist_ok=True)
    for pkg in pkgs:
        data = get_json(f"https://pypi.org/pypi/{pkg}/json")
        version = data["info"]["version"]
        files = data["urls"]
        wheels = [f for f in files if f["filename"].endswith(".whl")]
        win = [f for f in wheels if "win_amd64" in f["filename"] or "any" in f["filename"]]
        picked = None
        for pref in ("cp312-cp312-win_amd64", "abi3-win_amd64", "py3-none-any", "none-any"):
            for f in win:
                if pref in f["filename"]:
                    picked = f
                    break
            if picked:
                break
        if not picked:
            picked = (win or files or [None])[0]
        if not picked:
            print(f"{pkg}: no files found")
            continue
        dest = os.path.join(OUT, picked["filename"])
        if not os.path.exists(dest):
            print(f"{pkg} {version}: downloading {picked['filename']}")
            download(picked["url"], dest)
        else:
            print(f"{pkg} {version}: already have {picked['filename']}")
        print("   ->", dest)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
