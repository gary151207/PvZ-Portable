#!/usr/bin/env python3
"""Generate the fixed fifth-tier Bloomerang art from the supplied PvZ2 export."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "res/main/reanim"
DEFAULT_SOURCE = Path("/Users/fiture/Privates/pvz-2-crack/work/bloomerang")
SOURCE = Path("animations-json/1200/PLANT1")
SPRITE_SHA256 = "97d2a326c0660a92c8812debc0fac773c914fed3bfe82ec19bea5a727af1e900"
SPECS = (
    (SOURCE / "PLANT/BLOOMERANG/BLOOMERANG.json",
     "d691cb5672e8de584fb259c0a0a7978f4287ac03a8b78c9798ee05c092a8dfe4",
     "Bloomerang.reanim", "BLOOMERANG", 30, 274,
     (("anim_idle", tuple(range(0, 52))),
      ("anim_shooting", tuple(range(52, 92))),
      ("anim_water", tuple(range(216, 274)))), .48, 120., 115.),
    (SOURCE / "EFFECTS/BLOOMERANG_PROJECTILE_AVATAR/BLOOMERANG_PROJECTILE_AVATAR.json",
     "dba159d6f4e0374315570981a47f5bc5c318b9798140b69f49e0405a59d6cd80",
     "BloomerangProjectile.reanim", "BLOOMERANG_PROJECTILE", 24, 8,
     (("anim_fly", tuple(range(8))),), .32, 205., 205.),
    (SOURCE / "EFFECTS/BLOOMERANG_PROJECTILE/BLOOMERANG_PROJECTILE.json",
     "edf844c472b062077b78d19d36ab088b9d484f2a94509e3c46a88565ad9425cd",
     "BloomerangTornado.reanim", "BLOOMERANG_TORNADO", 24, 209,
     (("anim_spin", tuple(range(38, 95, 3))),), .24, 205., 205.),
    (SOURCE / "EFFECTS/BLOOMERANG_PROJECTILE_HIT/BLOOMERANG_PROJECTILE_HIT.json",
     "502fc976884463a533d40a41860b977dcc1cc09a2339e0342a125829e18d5713",
     "BloomerangHit.reanim", "BLOOMERANG_HIT", 30, 44,
     (("anim_hit", tuple(range(0, 11))),), .30, 205., 205.),
)

SHARED = Path(__file__).with_name("gen-spore-shroom-reanim.py")
spec = importlib.util.spec_from_file_location("pvz2_pam_converter", SHARED)
assert spec and spec.loader
pam = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = pam
spec.loader.exec_module(pam)


def generate(source: Path, item: tuple, resources: dict[str, Path]):
    path, digest, filename, prefix, fps, count, spans, scale, origin_x, origin_y = item
    raw = (source / path).read_bytes()
    if hashlib.sha256(raw).hexdigest() != digest:
        raise ValueError(f"Bloomerang PAM source hash changed: {path}")
    data = json.loads(raw)
    frames = data["main_sprite"]["frame"]
    if (data["version"], data["frame_rate"], len(frames)) != (6, fps, count):
        raise ValueError(f"unexpected Bloomerang PAM metadata: {path}")
    if filename == "Bloomerang.reanim":
        labels = tuple((i, frame["label"]) for i, frame in enumerate(frames) if frame.get("label"))
        actions = tuple(i for i, frame in enumerate(frames)
                        if any(c[0] == "use_action" for c in frame.get("command", [])))
        if labels != ((0, "idle"), (52, "attack"), (92, "attack_02"),
                      (117, "plantfoodON"), (135, "plantfood"),
                      (180, "plantfoodOFF"), (198, "plantfood_idle"), (216, "water")) or actions != (72, 160):
            raise ValueError("Bloomerang attack labels or action point changed")
    for timeline in [frames, *(sprite["frame"] for sprite in data["sprite"])]:
        for frame in timeline:
            for change in frame.get("change", []):
                color = change.get("color")
                if color is not None:
                    change["color"] = [1., 1., 1., color[3]]
    pam.EXCLUDED_SPRITE_NAMES = {s["name"] for s in data["sprite"]
                                 if "custom" in s["name"].lower()}
    children = [pam.simulate_timeline(s["frame"]) for s in data["sprite"]]
    main = pam.simulate_timeline(frames)
    conversion = pam.Conversion(path, filename, prefix, scale, origin_x, origin_y, {})
    selected = []
    for _, indices in spans:
        for index in indices:
            child_states = [timeline[index % len(timeline)] for timeline in children]
            leaves = pam.expand_frame(data, child_states, main[index], conversion)
            # The source tornado samples three zombie bodies. The actual target is
            # drawn by the game, so retain only the vortex and its debris layers.
            if filename == "BloomerangTornado.reanim":
                leaves = {key: value for key, value in leaves.items()
                          if value["image_index"] <= 35}
            selected.append(leaves)
    total = len(selected)
    tracks = [f"<fps>{fps}</fps>"]
    offset = 0
    for name, indices in spans:
        tracks.append(pam.marker_track(name, offset, offset + len(indices) - 1, total))
        offset += len(indices)
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
    generated = [(*generate(args.source, item, resources), item[2]) for item in SPECS]
    source_assets = {name: path for _, assets, _ in generated for path, name in assets}
    sprite_hash = hashlib.sha256()
    for name, path in sorted(source_assets.items()):
        sprite_hash.update(name.encode())
        sprite_hash.update(b"\0")
        sprite_hash.update(path.read_bytes())
    if sprite_hash.hexdigest() != SPRITE_SHA256:
        raise ValueError("Bloomerang source sprites hash changed")
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
        actual = {p.name for p in OUTPUT.glob("BLOOMERANG_*.png")}
        actual |= {p.name for p in OUTPUT.glob("Bloomerang*.reanim")}
        if actual != expected:
            print(f"unexpected Bloomerang files: {sorted(actual ^ expected)}", file=sys.stderr)
            ok = False
    print(f"Bloomerang resources {'match source' if args.check else 'generated'}: {len(expected)} files")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
