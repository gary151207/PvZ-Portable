#!/usr/bin/env python3
"""Generate fixed fifth-tier Oak Archer art from the supplied PvZ2 export."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "res/main/reanim"
DEFAULT_SOURCE = Path("/Users/fiture/Privates/pvz-2-crack/work/oakshooter")
SHARED = Path(__file__).with_name("gen-spore-shroom-reanim.py")
spec = importlib.util.spec_from_file_location("pvz2_pam_converter", SHARED)
assert spec and spec.loader
pam = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = pam
spec.loader.exec_module(pam)

PAM_PATH = Path("animations-json/1200/PLANT2/PLANT/OAKSHOOTER/OAKSHOOTER.json")
PAM_SHA256 = "b538e9debf55b0e69081538e1d9af550bf491ad4afce752eef947f2f583a9b48"
LABELS = ("idle", "idle2", "attack", "attack2", "plantfood", "attack_normal",
          "attack_power", "attack_multiple", "attack_track", "water")
SPANS = (("anim_idle", 0, 80), ("anim_idle2", 80, 197),
         ("anim_shooting", 197, 301), ("anim_shooting2", 301, 405),
         ("anim_water", 605, 665))
ARROWS = (("oakshooter_projectile.png", "OakArcherArrow.reanim", "OAK_ARROW"),
          ("oakshooter_projectile_super.png", "OakArcherFrostArrow.reanim", "OAK_FROST_ARROW"))


def plant_art(source: Path, resources: dict[str, Path]) -> tuple[bytes, list[tuple[Path, str]]]:
    raw = (source / PAM_PATH).read_bytes()
    if hashlib.sha256(raw).hexdigest() != PAM_SHA256:
        raise ValueError("Oak Archer PAM source hash changed")
    data = json.loads(raw)
    frames = data["main_sprite"]["frame"]
    labels = tuple(frame["label"] for frame in frames if frame.get("label"))
    actions = tuple((i, command[0]) for i, frame in enumerate(frames)
                    for command in frame.get("command", []))
    if (data["version"], data["frame_rate"], len(frames), labels) != (6, 30, 665, LABELS):
        raise ValueError("unexpected Oak Archer PAM metadata")
    if (269, "use_action") not in actions or (373, "use_action2") not in actions:
        raise ValueError("ordinary attack action points moved")

    pam.EXCLUDED_SPRITE_NAMES = {"_custom", "custom_01"}
    children = [pam.simulate_timeline(sprite["frame"]) for sprite in data["sprite"]]
    main = pam.simulate_timeline(frames)
    conversion = pam.Conversion(PAM_PATH, "OakArcher.reanim", "OAK_ARCHER", 0.48,
                                120.0, 115.0, {})
    selected = []
    for _, start, end in SPANS:
        for index in range(start, end):
            child_states = [timeline[index % len(timeline)] for timeline in children]
            selected.append(pam.expand_frame(data, child_states, main[index], conversion))

    count = len(selected)
    tracks = ["<fps>30</fps>"]
    offset = 0
    for name, start, end in SPANS:
        length = end - start
        tracks.append(pam.marker_track(name, offset, offset + length - 1, count))
        offset += length
    for layer in sorted({layer for frame in selected for layer in frame}):
        tracks.append(pam.visual_track("layer_" + "_".join(map(str, layer)),
                                       [frame.get(layer) for frame in selected], "OAK_ARCHER"))
    assets = []
    for index in sorted({state["image_index"] for frame in selected for state in frame.values()}):
        image_id = data["image"][index]["name"].split("|", 1)[1]
        assets.append((resources[image_id], f"OAK_ARCHER_{index:03d}.png"))
    return ("\n".join(tracks) + "\n").encode(), assets


def arrow_art(prefix: str) -> bytes:
    arrow_x = -44.5 if prefix == "OAK_ARROW" else -79.5
    state = {"image_index": 0, "x": arrow_x, "y": -17.0, "kx": 0.0,
             "ky": 0.0, "sx": 0.45, "sy": 0.45, "alpha": 1.0}
    tracks = ("<fps>30</fps>", pam.marker_track("anim_fly", 0, 1, 2),
              pam.visual_track("arrow", [state, state], prefix))
    return ("\n".join(tracks) + "\n").encode()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    resource_data = json.loads((args.source / "sprites/resources.json").read_text())
    resources = {item["id"]: args.source / "sprites" / Path(*item["path"]).with_suffix(".png")
                 for item in resource_data["resources"] if item["type"] == "Image"}
    if not args.check:
        OUTPUT.mkdir(parents=True, exist_ok=True)
    xml, assets = plant_art(args.source, resources)
    expected = {"OakArcher.reanim"}
    ok = pam.compare_or_write(OUTPUT / "OakArcher.reanim", xml, args.check)
    for path, name in assets:
        expected.add(name)
        ok = pam.install_asset(path, OUTPUT / name, args.check) and ok
    for source_name, filename, prefix in ARROWS:
        expected.update((filename, f"{prefix}_000.png"))
        ok = pam.compare_or_write(OUTPUT / filename, arrow_art(prefix), args.check) and ok
        path = args.source / "sprites/images/1200/plant2" / source_name
        ok = pam.install_asset(path, OUTPUT / f"{prefix}_000.png", args.check) and ok
    if args.check:
        actual = {p.name for p in OUTPUT.glob("OAK_*.png")}
        actual |= {p.name for p in OUTPUT.glob("OakArcher*.reanim")}
        if actual != expected:
            print(f"unexpected Oak Archer files: {sorted(actual ^ expected)}", file=sys.stderr)
            ok = False
    print(f"Oak Archer resources {'match source' if args.check else 'generated'}: {len(expected)} files")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
