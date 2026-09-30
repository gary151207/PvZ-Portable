#!/usr/bin/env python3
"""Generate fixed fourth-tier Pepper-pult art from the supplied PvZ2 export."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "res/main/reanim"
DEFAULT_SOURCE = Path("/Users/fiture/Privates/pvz-2-crack/work/pepperpult")
BASE = Path("animations-json/1200/PLANT2")
SPECS = (
    (BASE / "PLANT/PEPPERPULT/PEPPERPULT.json", "7bf4fdabaa6af4a860b1548a72bde40f977c7c11ffbbff99e85c6beebd951062",
     "PepperPult.reanim", "PEPPER_PULT", 753,
     (("anim_idle", 0, 100), ("anim_idle2", 100, 200), ("anim_idle3", 200, 268),
      ("anim_shooting", 268, 328), ("anim_water", 437, 497)), .48, 120., 115.),
    (BASE / "EFFECTS/PEPPERPULT_PROJECTILE2/PEPPERPULT_PROJECTILE2.json", "9e90c03efc82161e077f432c474ba3e06f55939701f971daaba508c7eae87543",
     "PepperPultProjectile.reanim", "PEPPER_PULT_PROJECTILE", 7,
     (("anim_fly", 0, 7),), .38, 195., 195.),
    (BASE / "EFFECTS/PEPPERPULT_PROJECTILE_SPLAT2/PEPPERPULT_PROJECTILE_SPLAT2.json", "7c65e3407d2ff4299c7e82581e6959632bd37c29a402ca07e8317b7f1920b9a4",
     "PepperPultHit.reanim", "PEPPER_PULT_HIT", 148,
     (("anim_hit", 37, 74),), .38, 195., 195.),
    (BASE / "EFFECTS/PEPPERPULT_BLUE_BURN/PEPPERPULT_BLUE_BURN.json", "b469b0635aed83295c892cf0f9862364df4ebdbe276490eda9f5b493fe2d54df",
     "PepperPultBlueBurn.reanim", "PEPPER_PULT_BURN", 156,
     (("anim_start", 0, 24), ("anim_burn", 24, 156)), .48, 195., 195.),
)

SHARED = Path(__file__).with_name("gen-spore-shroom-reanim.py")
spec = importlib.util.spec_from_file_location("pvz2_pam_converter", SHARED)
assert spec and spec.loader
pam = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = pam
spec.loader.exec_module(pam)


def generate(source: Path, entry: tuple, resources: dict[str, Path]):
    relative, digest, filename, prefix, count, spans, scale, ox, oy = entry
    raw = (source / relative).read_bytes()
    if hashlib.sha256(raw).hexdigest() != digest:
        raise ValueError(f"Pepper-pult PAM source hash changed: {relative}")
    data = json.loads(raw)
    frames = data["main_sprite"]["frame"]
    if (data["version"], data["frame_rate"], len(frames)) != (6, 30, count):
        raise ValueError(f"unexpected Pepper-pult PAM metadata: {relative}")
    labels = tuple((i, frame["label"]) for i, frame in enumerate(frames) if frame.get("label"))
    expected_labels = {
        "PepperPult.reanim": ((0, "idle"), (100, "idle2"), (200, "idle3"), (268, "attack"),
                                (328, "plantfood2"), (437, "water"), (644, "plantfood")),
        "PepperPultProjectile.reanim": ((0, "animation"),),
        "PepperPultHit.reanim": ((0, "animation"), (37, "animation2"),
                                   (74, "animation03"), (111, "animation04")),
        "PepperPultBlueBurn.reanim": ((0, "hit"), (24, "idle")),
    }
    if labels != expected_labels[filename]:
        raise ValueError(f"unexpected Pepper-pult labels: {relative}: {labels}")
    if filename == "PepperPult.reanim":
        actions = tuple(i for i, frame in enumerate(frames)
                        if any(command[0] == "use_action" for command in frame.get("command", [])))
        if actions != (281, 363, 378, 395, 410, 679, 694, 712):
            raise ValueError(f"Pepper-pult action points changed: {actions}")
    for timeline in [frames, *(sprite["frame"] for sprite in data["sprite"])]:
        for frame in timeline:
            for change in frame.get("change", []):
                color = change.get("color")
                if color is not None:
                    change["color"] = [1., 1., 1., color[3]]
    pam.EXCLUDED_SPRITE_NAMES = {sprite["name"] for sprite in data["sprite"]
                                 if "custom" in sprite["name"].lower()}
    children = [pam.simulate_timeline(sprite["frame"]) for sprite in data["sprite"]]
    main = pam.simulate_timeline(frames)
    conversion = pam.Conversion(relative, filename, prefix, scale, ox, oy, {})
    selected = []
    for _, start, end in spans:
        for index in range(start, end):
            child_states = [timeline[index % len(timeline)] for timeline in children]
            selected.append(pam.expand_frame(data, child_states, main[index], conversion))
    total = len(selected)
    tracks = ["<fps>30</fps>"]
    offset = 0
    for name, start, end in spans:
        length = end - start
        tracks.append(pam.marker_track(name, offset, offset + length - 1, total))
        offset += length
    for layer in sorted({layer for frame in selected for layer in frame}):
        tracks.append(pam.visual_track("layer_" + "_".join(map(str, layer)),
                                       [frame.get(layer) for frame in selected], prefix))
    assets = []
    for index in sorted({state["image_index"] for frame in selected for state in frame.values()}):
        image_id = data["image"][index]["name"].split("|", 1)[1]
        assets.append((resources[image_id], f"{prefix}_{index:03d}.png"))
    return ("\n".join(tracks) + "\n").encode(), assets


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    resource_data = json.loads((args.source / "sprites/resources.json").read_text())
    resources = {item["id"]: args.source / "sprites" / Path(*item["path"]).with_suffix(".png")
                 for item in resource_data["resources"] if item["type"] == "Image"}
    generated = [(*generate(args.source, entry, resources), entry[2]) for entry in SPECS]
    source_assets = {name: path for _, assets, _ in generated for path, name in assets}
    sprite_hash = hashlib.sha256()
    for name, path in sorted(source_assets.items()):
        sprite_hash.update(name.encode())
        sprite_hash.update(b"\0")
        sprite_hash.update(path.read_bytes())
    # Lock the exact subset of source sprites used by the four retained animations.
    expected_sprite_hash = "24dd058c99f8cba82640edce2b5d59a30ce2c73a85c2fc121b0852eb2e848f67"
    if sprite_hash.hexdigest() != expected_sprite_hash:
        raise ValueError("Pepper-pult source sprites hash changed")
    if not args.check:
        OUTPUT.mkdir(parents=True, exist_ok=True)
    expected = set()
    ok = True
    for xml, assets, filename in generated:
        expected.add(filename)
        ok = pam.compare_or_write(OUTPUT / filename, xml, args.check) and ok
        for source_file, name in assets:
            expected.add(name)
            ok = pam.install_asset(source_file, OUTPUT / name, args.check) and ok
    if args.check:
        actual = {p.name for p in OUTPUT.glob("PEPPER_PULT_*.png")}
        actual |= {p.name for p in OUTPUT.glob("PepperPult*.reanim")}
        if actual != expected:
            print(f"unexpected Pepper-pult files: {sorted(actual ^ expected)}", file=sys.stderr)
            ok = False
    print(f"Pepper-pult resources {'match source' if args.check else 'generated'}: {len(expected)} files; source sprites {sprite_hash.hexdigest()}")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
