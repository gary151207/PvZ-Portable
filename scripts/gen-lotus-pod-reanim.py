#!/usr/bin/env python3
"""Generate the normal, fixed-level Lotus Pod art from the supplied PvZ2 export."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "res/main/reanim"
DEFAULT_SOURCE = Path("/Users/fiture/Privates/pvz-2-crack/work/lotusshower")
SHARED = Path(__file__).with_name("gen-spore-shroom-reanim.py")
spec = importlib.util.spec_from_file_location("pvz2_pam_converter", SHARED)
assert spec and spec.loader
pam = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = pam
spec.loader.exec_module(pam)

BASE = Path("animations-json/1200/PLANT2")
WORK = (
    (BASE / "PLANT/LOTUSHOOTER/LOTUSHOOTER.json", "LotusPod.reanim", "LOTUSPOD",
     (("anim_idle", 0, 60), ("anim_idle2", 60, 148), ("anim_shooting", 148, 231),
      ("anim_water", 329, 389)), 25, 0.52, 100.0, 110.0,
     "926b50caf23948574f009c8e3a32efa8c6c8f0e18de936dabc34b0b4ca6c003a"),
    (BASE / "EFFECTS/LOTUSHOOTER_PLANTFOOD_LAND/LOTUSHOOTER_PLANTFOOD_LAND.json",
     "LotusPodSeed.reanim", "LOTUSPOD_SEED", (("anim_fly", 0, 40),),
     24, 0.55, 195.0, 195.0, "96a6fafcaec54cda42479141c5b861eb9e3441ec21061ccee838ee5f8ad87eaf"),
    (BASE / "EFFECTS/LOTUSHOOTER_PLANTFOOD_WATER/LOTUSHOOTER_PLANTFOOD_WATER.json",
     "LotusPodTorpedo.reanim", "LOTUSPOD_TORPEDO", (("anim_fly", 0, 36),),
     24, 0.55, 195.0, 195.0, "8490c65d5623ed24d2776087fe88f04d24951807368271f38c8bd237454abba0"),
    (BASE / "EFFECTS/LOTUSHOOTER_HIT/LOTUSHOOTER_HIT.json", "LotusPodHit.reanim",
     "LOTUSPOD_HIT", (("anim_hit", 0, 35),), 30, 0.55, 195.0, 195.0,
     "42baadac3cea7ee6f615965aca20d37c65f639c4ff0413c3fca9e6a4c624df0c"),
    (BASE / "EFFECTS/LOTUSHOOTER_VORTEX/LOTUSHOOTER_VORTEX.json", "LotusPodVortex.reanim",
     "LOTUSPOD_VORTEX", (("anim_stun", 0, 32),), 30, 0.55, 195.0, 195.0,
     "1250a0715f4cccc5cbf4ff5bde8a4bd5d3a778c70e03bf2bb66c76a3728e6b85"),
)

LABELS = (
    ("idle", "idlefree", "attack", "plantfood", "water"),
    ("normal", "plantfood", "avatar"),
    ("waterbomb", "waterbomb_plantfood", "waterbomb_avatar"),
    ("animation",),
    ("animation",),
)


def generate(source: Path, entry: tuple, resources: dict[str, Path], labels: tuple[str, ...]):
    relative, filename, prefix, spans, fps, scale, ox, oy, expected_hash = entry
    raw = (source / relative).read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != expected_hash:
        raise ValueError(f"source hash changed: {relative}: {digest}")
    data = json.loads(raw)
    frames = data["main_sprite"]["frame"]
    actual_labels = tuple(frame["label"] for frame in frames if frame.get("label"))
    if data["version"] != 6 or data["frame_rate"] != fps or actual_labels != labels:
        raise ValueError(f"unexpected PAM metadata: {relative}: {actual_labels}")
    if filename == "LotusPod.reanim" and not any(
        command[0] == "use_action" for command in frames[159]["command"]
    ):
        raise ValueError("normal attack action point moved from source frame 159")

    # The normal torpedo segment contains one coloured highlight. Reanim supports
    # alpha and geometry, but not per-track RGB modulation.
    for timeline in [frames, *(sprite["frame"] for sprite in data["sprite"])]:
        for frame in timeline:
            for change in frame.get("change", []):
                color = change.get("color")
                if color is not None:
                    change["color"] = [1.0, 1.0, 1.0, color[3]]

    pam.EXCLUDED_SPRITE_NAMES = {"_custom", "custom_01"}
    children = [pam.simulate_timeline(sprite["frame"]) for sprite in data["sprite"]]
    main = pam.simulate_timeline(frames)
    conversion = pam.Conversion(relative, filename, prefix, scale, ox, oy, {})
    selected = []
    for _, start, end in spans:
        for frame_index in range(start, end):
            # PAM child sprites advance independently. One Lotus Pod pupil sprite
            # has a 67-frame timeline; use its current frame instead of frame zero.
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

    image_indices = sorted({state["image_index"] for frame in selected for state in frame.values()})
    assets = []
    for index in image_indices:
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
    resource_data = json.loads((args.source / "sprites/resources.json").read_text())
    resources = {
        item["id"]: args.source / "sprites" / Path(*item["path"]).with_suffix(".png")
        for item in resource_data["resources"] if item["type"] == "Image"
    }
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
        actual = {p.name for p in OUTPUT.glob("LOTUSPOD_*.png")}
        actual |= {p.name for p in OUTPUT.glob("LotusPod*.reanim")}
        if actual != expected:
            print(f"unexpected Lotus Pod files: {sorted(actual ^ expected)}", file=sys.stderr)
            ok = False
    print(f"Lotus Pod resources {'match source' if args.check else 'generated'}: {len(expected)} files")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
