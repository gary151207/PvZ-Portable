#!/usr/bin/env python3
"""Generate fixed-level Carrotillery's normal attack art from its PAM export."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "res/main/reanim"
DEFAULT_SOURCE = Path("/Users/fiture/Privates/pvz-2-crack/work/carrotlauncher")
SHARED = Path(__file__).with_name("gen-spore-shroom-reanim.py")
spec = importlib.util.spec_from_file_location("pvz2_pam_converter", SHARED)
assert spec and spec.loader
pam = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = pam
spec.loader.exec_module(pam)

BASE = Path("animations-json/1200/PLANT2")
WORK = (
    (BASE / "PLANT/CARROTLAUNCHER/CARROTLAUNCHER.json", "Carrotillery.reanim", "CARROTILLERY",
     (("anim_idle", 0, 140), ("anim_idle2", 993, 1133),
      ("anim_idle3", 1133, 1273), ("anim_idle4", 1273, 1413),
      ("anim_volley", 140, 504), ("anim_recover", 504, 555),
      ("anim_recover2", 555, 586)), 30, 0.50, 120.0, 110.0,
     "e7ea7f49111086bcf4d894fa145791b7f19df8cd0c6315ddc2b54053333aeacf"),
    (BASE / "EFFECTS/CARROT_BULLET/CARROT_BULLET.json", "CarrotilleryBullet.reanim", "CARROT_BULLET",
     (("anim_fly", 0, 21),), 24, 0.60, 180.0, 180.0,
     "9acc2fddce1a7c2656f7f231d7edeae61a7b61e789bf7b26370882ea2ea28407"),
    (BASE / "EFFECTS/CARROT_BULLET_HIT/CARROT_BULLET_HIT.json", "CarrotilleryHit.reanim", "CARROT_BULLET_HIT",
     (("anim_hit", 0, 20),), 24, 0.60, 180.0, 180.0,
     "6b37518cd3a31b15ee6962742f555e78b154fa193bebe3ef92924d4e8bb860fd"),
)
LABELS = (
    ("idle", "attack", "attack2", "attack3", "attack4", "recover", "recover2",
     "attack_lv2", "attack2_lv2", "attack_lv3", "idle2", "idle3", "idle4",
     "plantfood", "plantfood2", "water"),
    ("anim",),
    ("anim",),
)
ACTION_FRAMES = (188, 275, 366, 459)


def generate(source: Path, entry: tuple, resources: dict[str, Path], labels: tuple[str, ...]):
    relative, filename, prefix, spans, fps, scale, ox, oy, expected_hash = entry
    raw = (source / relative).read_bytes()
    if hashlib.sha256(raw).hexdigest() != expected_hash:
        raise ValueError(f"source hash changed: {relative}")
    data = json.loads(raw)
    frames = data["main_sprite"]["frame"]
    actual_labels = tuple(frame["label"] for frame in frames if frame.get("label"))
    if data["version"] != 6 or data["frame_rate"] != fps or actual_labels != labels:
        raise ValueError(f"unexpected PAM metadata: {relative}: {actual_labels}")
    if filename == "Carrotillery.reanim":
        actions = tuple(i for i, frame in enumerate(frames)
                        if any(command[0] == "use_action" for command in frame.get("command", [])))
        if actions[:4] != ACTION_FRAMES:
            raise ValueError(f"ordinary attack action points changed: {actions}")

    # Reanim supports per-frame alpha but has no per-track RGB multiplier.
    for timeline in [frames, *(sprite["frame"] for sprite in data["sprite"])]:
        for frame in timeline:
            for change in frame.get("change", []):
                color = change.get("color")
                if color is not None:
                    if color[0] != color[1] or color[1] != color[2]:
                        raise ValueError("unsupported non-grayscale RGB animation")
                    change["color"] = [1.0, 1.0, 1.0, color[3]]

    pam.EXCLUDED_SPRITE_NAMES = {"_custom", "custom_01"}
    children = [pam.simulate_timeline(sprite["frame"]) for sprite in data["sprite"]]
    main = pam.simulate_timeline(frames)
    conversion = pam.Conversion(relative, filename, prefix, scale, ox, oy, {})
    selected = []
    for _, start, end in spans:
        for frame_index in range(start, end):
            child_states = [timeline[frame_index % len(timeline)] for timeline in children]
            selected.append(pam.expand_frame(data, child_states, main[frame_index], conversion))

    count = len(selected)
    tracks = [f"<fps>{fps}</fps>"]
    offset = 0
    for name, start, end in spans:
        length = end - start
        tracks.append(pam.marker_track(name, offset, offset + length - 1, count))
        offset += length
    for layer in sorted({layer for frame in selected for layer in frame}):
        tracks.append(pam.visual_track("layer_" + "_".join(map(str, layer)),
                                       [frame.get(layer) for frame in selected], prefix))

    assets = []
    for index in sorted({state["image_index"] for frame in selected for state in frame.values()}):
        image_id = data["image"][index]["name"].split("|", 1)[1]
        if image_id not in resources:
            raise ValueError(f"missing sprite resource ID {image_id}")
        assets.append((resources[image_id], f"{prefix}_{index:03d}.png"))
    return ("\n".join(tracks) + "\n").encode(), assets


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    resources = {}
    data = json.loads((args.source / "sprites/carrotlauncher/resources.json").read_text())
    for item in data["resources"]:
        if item["type"] == "Image" and not item.get("atlas"):
            resources[item["id"]] = args.source / "sprites/carrotlauncher" / Path(*item["path"]).with_suffix(".png")
    if not args.check:
        OUTPUT.mkdir(parents=True, exist_ok=True)
    expected = set()
    ok = True
    for entry, labels in zip(WORK, LABELS):
        xml, assets = generate(args.source, entry, resources, labels)
        expected.add(entry[1])
        ok = pam.compare_or_write(OUTPUT / entry[1], xml, args.check) and ok
        for source_file, name in assets:
            expected.add(name)
            ok = pam.install_asset(source_file, OUTPUT / name, args.check) and ok
    if args.check:
        actual = {p.name for p in OUTPUT.glob("CARROTILLERY_*.png")}
        actual |= {p.name for p in OUTPUT.glob("CARROT_BULLET_*.png")}
        actual |= {p.name for p in OUTPUT.glob("CARROT_BULLET_HIT_*.png")}
        actual |= {p.name for p in OUTPUT.glob("CARROT_HIT_*.png")}
        actual |= {p.name for p in OUTPUT.glob("Carrotillery*.reanim")}
        if actual != expected:
            print(f"unexpected Carrotillery files: {sorted(actual ^ expected)}", file=sys.stderr)
            ok = False
    print(f"Carrotillery resources {'match source' if args.check else 'generated'}: {len(expected)} files")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
