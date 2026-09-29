#!/usr/bin/env python3
"""Generate fixed fifth-tier Bonk Choy art from the supplied PvZ2 export."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "res/main/reanim"
DEFAULT_SOURCE = Path("/Users/fiture/Privates/pvz-2-crack/work/bonkchoy")
PAM_PATH = Path("animations-json/1200/PLANT1/PLANT/BONKCHOY/BONKCHOY.json")
PAM_SHA256 = "f50e54be4aaa5382eb4067771ddad6fca3704185b2e398ac53c17eaf5eb468d0"
SPANS = (
    ("anim_idle", 0, 30),
    ("anim_idle2", 30, 60),
    ("anim_idle3", 60, 91),
    ("anim_punch", 91, 101),
    ("anim_punch_left", 101, 111),
    ("anim_uppercut", 131, 146),
    ("anim_uppercut_left", 146, 161),
    ("anim_water", 301, 353),
    ("anim_quake", 353, 403),
)
LABELS = ("idle", "idle2", "idle3", "attack", "attack2", "attack3", "attack4",
          "attack5", "plantfood_on", "plantfood", "plantfood_off", "plantfood_on2",
          "plantfood2", "plantfood_off2", "water", "attack6", "attack7")
ACTIONS = (95, 105, 115, 125, 135, 150, 378, 428)

SHARED = Path(__file__).with_name("gen-spore-shroom-reanim.py")
spec = importlib.util.spec_from_file_location("pvz2_pam_converter", SHARED)
assert spec and spec.loader
pam = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = pam
spec.loader.exec_module(pam)


def generate(source: Path, resources: dict[str, Path]) -> tuple[bytes, list[tuple[Path, str]]]:
    raw = (source / PAM_PATH).read_bytes()
    if hashlib.sha256(raw).hexdigest() != PAM_SHA256:
        raise ValueError("Bonk Choy PAM source hash changed")
    data = json.loads(raw)
    frames = data["main_sprite"]["frame"]
    labels = tuple(frame["label"] for frame in frames if frame.get("label"))
    actions = tuple(i for i, frame in enumerate(frames)
                    if any(command[0] == "use_special" for command in frame.get("command", [])))
    if (data["version"], data["frame_rate"], len(frames), labels, actions) != \
            (6, 30, 453, LABELS, ACTIONS):
        raise ValueError("unexpected Bonk Choy PAM metadata")

    # Reanim supports per-track alpha but not RGB multipliers. Retain the
    # source opacity and omit the unsupported tint animation.
    for timeline in [frames, *(sprite["frame"] for sprite in data["sprite"])]:
        for frame in timeline:
            for change in frame.get("change", []):
                color = change.get("color")
                if color is not None:
                    change["color"] = [1.0, 1.0, 1.0, color[3]]

    pam.EXCLUDED_SPRITE_NAMES = {sprite["name"] for sprite in data["sprite"]
                                 if "custom" in sprite["name"].lower()}
    children = [pam.simulate_timeline(sprite["frame"]) for sprite in data["sprite"]]
    main = pam.simulate_timeline(frames)
    conversion = pam.Conversion(PAM_PATH, "BonkChoy.reanim", "BONK_CHOY", 0.48,
                                120.0, 115.0, {})
    selected = []
    for _, start, end in SPANS:
        for index in range(start, end):
            child_states = [timeline[index % len(timeline)] for timeline in children]
            selected.append(pam.expand_frame(data, child_states, main[index], conversion))

    frame_count = len(selected)
    tracks = ["<fps>30</fps>"]
    offset = 0
    for name, start, end in SPANS:
        length = end - start
        tracks.append(pam.marker_track(name, offset, offset + length - 1, frame_count))
        offset += length
    for layer in sorted({layer for frame in selected for layer in frame}):
        tracks.append(pam.visual_track("layer_" + "_".join(map(str, layer)),
                                       [frame.get(layer) for frame in selected], "BONK_CHOY"))

    assets = []
    for index in sorted({state["image_index"] for frame in selected for state in frame.values()}):
        image_id = data["image"][index]["name"].split("|", 1)[1]
        assets.append((resources[image_id], f"BONK_CHOY_{index:03d}.png"))
    return ("\n".join(tracks) + "\n").encode(), assets


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
    xml, assets = generate(args.source, resources)
    expected = {"BonkChoy.reanim"}
    ok = pam.compare_or_write(OUTPUT / "BonkChoy.reanim", xml, args.check)
    for source_file, name in assets:
        expected.add(name)
        ok = pam.install_asset(source_file, OUTPUT / name, args.check) and ok
    if args.check:
        actual = {path.name for path in OUTPUT.glob("BONK_CHOY_*.png")}
        actual |= {path.name for path in OUTPUT.glob("BonkChoy*.reanim")}
        if actual != expected:
            print(f"unexpected Bonk Choy files: {sorted(actual ^ expected)}", file=sys.stderr)
            ok = False
    print(f"Bonk Choy resources {'match source' if args.check else 'generated'}: {len(expected)} files")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
