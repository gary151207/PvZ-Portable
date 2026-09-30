#!/usr/bin/env python3
"""Generate fixed fourth-tier Celery Stalker art from the supplied PvZ2 export."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "res/main/reanim"
DEFAULT_SOURCE = Path("/Users/fiture/Privates/pvz-2-crack/work/celerystalker")
PLANT = Path("animations-json/1200/PLANT1/PLANT/CELERYSTALKER/CELERYSTALKER.json")
EFFECT = Path("animations-json/1200/PLANT1/EFFECTS/CELERYSTALKER_RE/CELERYSTALKER_RE.json")
HASHES = {
    PLANT: "b0d3717b138bee824fe2271a93d5ab5a9b1c270b6052d888f324c2e0dce3a110",
    EFFECT: "e58e583bfe0a78005e46bc9d8dc366bf5bf3fcc0fdbcfa4646e6cd04ff4daa64",
}
LABELS = ("idle", "idle2", "idle3", "idle4", "down", "idle_down", "idle_down2",
          "idle_down3", "up", "attack", "attack_loop", "attack_end", "plantfood_on",
          "plantfood", "plantfood_off", "water", "attack_special")
SPANS = (
    ("anim_idle", 0, 50),
    ("anim_down", 200, 232),
    ("anim_idle_down", 232, 282),
    ("anim_up", 382, 414),
    ("anim_attack_start", 414, 422),
    ("anim_attack_loop", 422, 443),
    ("anim_attack_end", 443, 461),
    ("anim_attack_special", 549, 573),
)
HITS = (423, 426, 429, 432, 435, 438, 441)

spec = importlib.util.spec_from_file_location("pvz2_pam_converter", Path(__file__).with_name("gen-spore-shroom-reanim.py"))
assert spec and spec.loader
pam = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = pam
spec.loader.exec_module(pam)


def source_data(source: Path, path: Path) -> dict:
    raw = (source / path).read_bytes()
    if hashlib.sha256(raw).hexdigest() != HASHES[path]:
        raise ValueError(f"Celery Stalker PAM source changed: {path}")
    data = json.loads(raw)
    if data["version"] != 6 or data["frame_rate"] != 30:
        raise ValueError(f"unexpected PAM version or FPS: {path}")
    return data


def write_reanim(source: Path, data: dict, path: Path, spans: tuple,
                 name: str, prefix: str, scale: float, origin_x: float,
                 origin_y: float) -> tuple[bytes, list[tuple[Path, str]]]:
    frames = data["main_sprite"]["frame"]
    if path == PLANT:
        labels = tuple(frame["label"] for frame in frames if frame.get("label"))
        hits = tuple(i for i, frame in enumerate(frames)
                     if any(command[0] == "use_special" for command in frame.get("command", [])))
        palm = tuple(i for i, frame in enumerate(frames)
                     if any(command[0] == "use_action" for command in frame.get("command", [])))
        if (len(frames), labels, hits, palm) != (573, LABELS, HITS, (558,)):
            raise ValueError("unexpected Celery Stalker labels or action points")
    elif len(frames) != 22 or frames[0].get("label") != "re":
        raise ValueError("unexpected Celery Stalker effect timeline")

    # The effect uses a yellow RGB tint that reanim cannot express per frame.
    # Retain source opacity; the runtime applies a fixed yellow tint to it.
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
    conversion = pam.Conversion(path, name, prefix, scale, origin_x, origin_y, {})
    selected = []
    for _, start, end in spans:
        for index in range(start, end):
            child_states = [timeline[index % len(timeline)] for timeline in children]
            selected.append(pam.expand_frame(data, child_states, main[index], conversion))

    tracks = ["<fps>30</fps>"]
    offset = 0
    for marker, start, end in spans:
        length = end - start
        tracks.append(pam.marker_track(marker, offset, offset + length - 1, len(selected)))
        offset += length
    for layer in sorted({layer for frame in selected for layer in frame}):
        tracks.append(pam.visual_track("layer_" + "_".join(map(str, layer)),
                                       [frame.get(layer) for frame in selected], prefix))

    resources = json.loads((source / "sprites/resources.json").read_text())
    images = {item["id"]: source / "sprites" / Path(*item["path"]).with_suffix(".png")
              for item in resources["resources"] if item["type"] == "Image"}
    assets = []
    for index in sorted({state["image_index"] for frame in selected for state in frame.values()}):
        image_id = data["image"][index]["name"].split("|", 1)[1]
        assets.append((images[image_id], f"{prefix}_{index:03d}.png"))
    return ("\n".join(tracks) + "\n").encode(), assets


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    plant = source_data(args.source, PLANT)
    effect = source_data(args.source, EFFECT)
    outputs = (
        write_reanim(args.source, plant, PLANT, SPANS, "CeleryStalker.reanim", "CELERY_STALKER", 0.42, 100.0, 55.0),
        write_reanim(args.source, effect, EFFECT, (("anim_re", 0, 22),),
                     "CeleryStalkerEffect.reanim", "CELERY_STALKER_EFFECT", 0.42, 195.0, 195.0),
    )
    if not args.check:
        OUTPUT.mkdir(parents=True, exist_ok=True)
    expected = {"CeleryStalker.reanim", "CeleryStalkerEffect.reanim"}
    ok = True
    for (xml, assets), name in zip(outputs, ("CeleryStalker.reanim", "CeleryStalkerEffect.reanim")):
        ok = pam.compare_or_write(OUTPUT / name, xml, args.check) and ok
        for original, filename in assets:
            expected.add(filename)
            ok = pam.install_asset(original, OUTPUT / filename, args.check) and ok
    if args.check:
        actual = {p.name for p in OUTPUT.glob("CELERY_STALKER_*.png")}
        actual |= {p.name for p in OUTPUT.glob("CeleryStalker*.reanim")}
        if actual != expected:
            print(f"unexpected Celery Stalker files: {sorted(actual ^ expected)}", file=sys.stderr)
            ok = False
    print(f"Celery Stalker resources {'match source' if args.check else 'generated'}: {len(expected)} files")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
