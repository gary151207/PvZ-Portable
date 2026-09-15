#!/usr/bin/env python3
"""Minimal read-only reader for PvZ-Portable .pak archives.

Handy when wiring up player-supplied art (e.g. the Electric Gatling Pea /
Electric Starfruit sprites): list what a main.pak actually contains and pull a
single entry out for inspection.

Pak layout (all little-endian), matching src/SexyAppFramework/paklib/PakInterface.cpp:

    uint32 magic   = 0xBAC04AC0
    uint32 version = 0
    repeated entry:
        uint8  flags      (bit 0x80 set on the terminator entry)
        uint8  nameWidth
        char   name[nameWidth]   ('\\' normalised to '/')
        int32  srcSize
        int64  fileTime
    <data blob: every entry's bytes, concatenated in entry order>

The whole file body is XORed with 0xF7 on load, so this tool decodes first.

Usage:
    python tools/pvzp-pak.py list    <pak> [substring]
    python tools/pvzp-pak.py extract <pak> <entry> <outfile>
"""

import struct
import sys

MAGIC = 0xBAC04AC0
XOR_KEY = 0xF7


def decode(raw):
    return bytes(b ^ XOR_KEY for b in raw)


def parse(dec):
    magic, version = struct.unpack_from("<II", dec, 0)
    if magic != MAGIC:
        raise SystemExit(f"bad magic 0x{magic:08X} (not a PvZ pak?)")
    if version != 0:
        raise SystemExit(f"unsupported pak version {version}")

    pos = 8
    entries = []
    while True:
        flags = dec[pos]
        pos += 1
        if flags & 0x80:
            break
        name_width = dec[pos]
        pos += 1
        name = dec[pos:pos + name_width].decode("latin-1").replace("\\", "/")
        pos += name_width
        (src_size,) = struct.unpack_from("<i", dec, pos)
        pos += 4
        (file_time,) = struct.unpack_from("<q", dec, pos)
        pos += 8
        entries.append({"name": name, "size": src_size, "time": file_time})

    offset = pos
    for entry in entries:
        entry["offset"] = offset
        offset += entry["size"]
    return entries


def read_pak(path):
    with open(path, "rb") as handle:
        dec = decode(handle.read())
    return dec, parse(dec)


def cmd_list(argv):
    dec, entries = read_pak(argv[0])
    needle = argv[1].lower() if len(argv) > 1 else None
    for entry in entries:
        if needle and needle not in entry["name"].lower():
            continue
        print(f"{entry['size']:>9}  {entry['name']}")
    print(f"({len(entries)} entries)")


def cmd_extract(argv):
    dec, entries = read_pak(argv[0])
    wanted = argv[1].replace("\\", "/").lower()
    for entry in entries:
        if entry["name"].lower() == wanted:
            with open(argv[2], "wb") as handle:
                handle.write(dec[entry["offset"]:entry["offset"] + entry["size"]])
            print(f"wrote {argv[2]} ({entry['size']} bytes)")
            return
    raise SystemExit(f"entry not found: {argv[1]}")


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    command, rest = sys.argv[1], sys.argv[2:]
    if command == "list":
        cmd_list(rest)
    elif command == "extract":
        if len(rest) != 3:
            raise SystemExit("extract needs <pak> <entry> <outfile>")
        cmd_extract(rest)
    else:
        raise SystemExit(f"unknown command {command}\n{__doc__}")


if __name__ == "__main__":
    main()
