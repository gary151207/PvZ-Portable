"""Extract embedded font assets (Font / TMP font data) from the game's Unity data files.

Usage:
    python extract_fonts.py <input-file> [more-files ...] --out <dir>
"""
from __future__ import annotations

import argparse
import json
import os
import sys
import traceback

FONT_SIGS = {
    b"\x00\x01\x00\x00": "ttf",
    b"true": "ttf",
    b"ttcf": "ttc",
    b"OTTO": "otf",
    b"\x00\x01\x00\x00\x00": "ttf",
}


def as_bytes(value) -> bytes:
    """UnityPy exposes char[] fields as a list of ints; normalise everything to bytes."""
    if value is None:
        return b""
    if isinstance(value, (bytes, bytearray)):
        return bytes(value)
    if isinstance(value, str):
        return value.encode("utf-8", "surrogateescape")
    if isinstance(value, (list, tuple)):
        return bytes(bytearray(value))
    try:
        return bytes(value)
    except Exception:  # noqa: BLE001
        return b""


def sig_of(data: bytes) -> str | None:
    if not data or len(data) < 4:
        return None
    head = data[:4]
    if head == b"OTTO":
        return "otf"
    if head == b"ttcf":
        return "ttc"
    if head in (b"\x00\x01\x00\x00", b"true", b"typ1"):
        return "ttf"
    if data[:5] == b"\x00\x01\x00\x00\x00":
        return "ttf"
    return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("inputs", nargs="+")
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    import UnityPy

    os.makedirs(args.out, exist_ok=True)
    report = []
    error_count = [0]

    for path in args.inputs:
        print(f"==== loading {path}", flush=True)
        try:
            env = UnityPy.load(path)
        except Exception:
            traceback.print_exc()
            continue

        count = 0
        for obj in env.objects:
            count += 1
            if count % 20000 == 0:
                print(f"  ... {count} objects scanned", flush=True)
            tname = obj.type.name
            try:
                if tname == "Font":
                    data = obj.read()
                    name = getattr(data, "m_Name", None) or f"font_{obj.path_id}"
                    fdata = as_bytes(getattr(data, "m_FontData", None))
                    entry = {
                        "source": os.path.basename(path),
                        "path_id": obj.path_id,
                        "type": "Font",
                        "name": name,
                        "font_names": list(getattr(data, "m_FontNames", []) or []),
                        "size": len(fdata),
                    }
                    if fdata:
                        ext = sig_of(fdata) or "ttf"
                        out = os.path.join(args.out, sanitize(name) + "." + ext)
                        with open(out, "wb") as fh:
                            fh.write(fdata)
                        entry["file"] = os.path.basename(out)
                    report.append(entry)
                    print(f"  [Font] {name} -> {entry.get('file')} ({entry['size']} bytes)", flush=True)

                elif tname == "TextAsset":
                    data = obj.read()
                    raw = as_bytes(getattr(data, "m_Script", None))
                    if raw:
                        ext = sig_of(raw[:8])
                        if ext:
                            name = getattr(data, "m_Name", None) or f"text_{obj.path_id}"
                            out = os.path.join(args.out, sanitize(name) + "." + ext)
                            with open(out, "wb") as fh:
                                fh.write(raw)
                            report.append({
                                "source": os.path.basename(path),
                                "path_id": obj.path_id,
                                "type": "TextAsset",
                                "name": name,
                                "size": len(raw),
                                "file": os.path.basename(out),
                            })
                            print(f"  [TextAsset font] {name} ({len(raw)} bytes)", flush=True)

                elif tname == "MonoBehaviour":
                    # TextMeshPro font assets keep the source TTF in a referenced Font
                    # object, but some mods store raw font bytes on the behaviour itself.
                    data = obj.read()
                    for attr in ("m_FontData", "fontData", "m_SourceFontData"):
                        raw = as_bytes(getattr(data, attr, None))
                        if raw and sig_of(raw[:8]):
                            name = getattr(data, "m_Name", None) or f"mono_{obj.path_id}"
                            out = os.path.join(args.out, sanitize(f"{name}_{attr}") + "." + sig_of(raw[:8]))
                            with open(out, "wb") as fh:
                                fh.write(raw)
                            report.append({
                                "source": os.path.basename(path),
                                "path_id": obj.path_id,
                                "type": f"MonoBehaviour.{attr}",
                                "name": name,
                                "size": len(raw),
                                "file": os.path.basename(out),
                            })
                            print(f"  [MonoBehaviour font] {name} ({len(raw)} bytes)", flush=True)
                            break
            except Exception as exc:  # noqa: BLE001
                if error_count[0] < 200:
                    error_count[0] += 1
                    report.append({
                        "source": os.path.basename(path),
                        "path_id": getattr(obj, "path_id", None),
                        "type": tname,
                        "error": f"{type(exc).__name__}: {exc}",
                    })

        print(f"  scanned {count} objects in {os.path.basename(path)}", flush=True)

    with open(os.path.join(args.out, "fonts-report.json"), "w", encoding="utf-8") as fh:
        json.dump(report, fh, ensure_ascii=False, indent=2)
    print("done; report written", flush=True)
    return 0


def sanitize(name: str) -> str:
    bad = '<>:"/\\|?*'
    out = "".join("_" if c in bad else c for c in str(name)).strip()
    return out or "unnamed"


if __name__ == "__main__":
    sys.exit(main())
