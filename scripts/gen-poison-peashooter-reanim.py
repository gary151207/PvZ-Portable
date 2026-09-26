#!/usr/bin/env python3
"""Generate the level-one Poison Peashooter reanims from the supplied PvZ2 export."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = Path("/Users/fiture/Privates/pvz-2-crack/work/poisonpeashooter")
OUTPUT = ROOT / "res/main/reanim"
SHARED_SCRIPT = Path(__file__).with_name("gen-spore-shroom-reanim.py")
spec = importlib.util.spec_from_file_location("pvz2_pam_converter", SHARED_SCRIPT)
assert spec and spec.loader
pam = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = pam
spec.loader.exec_module(pam)

PLANT_JSON = Path("animations-json/1200/PLANT3/PLANT/POISONPEASHOOTER/POISONPEASHOOTER.json")
PROJECTILE_JSON = Path("animations-json/1200/PLANT3/EFFECTS/POISONPEASHOOTER_PROJECTILES/POISONPEASHOOTER_PROJECTILES.json")
SOURCE_HASHES = {
    PLANT_JSON: "4eb7114db1b76ec202b51cdee868f337c6f052d8c9a4c9934e05eb10679edcd4",
    PROJECTILE_JSON: "e0c04732b3da4bbb66af634f09363ccc5e209e97c862eef80277723a09408bdd",
}
PLANT_LABELS = [("anim_idle", 0, 40), ("anim_idle2", 40, 80),
                ("anim_idle3", 80, 144), ("anim_shooting", 144, 192)]
PROJECTILE_LABELS = [("anim_fly", 0, 9), ("anim_hit", 27, 42)]
EXCLUDED_SPRITES = {
    "custom_01", "custom_02", "custom_03a", "custom_03b", "_custom",
    "_peashooter_head_01_custom03",
}


def generate(source: Path, relative_json: Path, filename: str, prefix: str,
             labels: list[tuple[str, int, int]], scale: float,
             origin_x: float, origin_y: float) -> tuple[bytes, list[tuple[Path, str]]]:
    import json

    path = source / relative_json
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if digest != SOURCE_HASHES[relative_json]:
        raise ValueError(f"unexpected source hash: {relative_json}: {digest}")
    data = json.loads(path.read_text(encoding="utf-8"))
    if data["version"] != 6 or data["frame_rate"] != 30:
        raise ValueError("expected PAM version 6 at 30 FPS")
    if any(len(sprite["frame"]) != 1 for sprite in data["sprite"]):
        raise ValueError("unexpected multi-frame child sprite")

    source_labels = [(i, frame["label"]) for i, frame in enumerate(data["main_sprite"]["frame"])
                     if frame.get("label")]
    expected_labels = (["idle", "idle2", "idle3", "attack", "plantfood", "attack5", "water"]
                       if relative_json == PLANT_JSON else
                       ["projectile_t1", "projectile_t2", "projectile_t3",
                        "hit_t1", "hit_t2", "hit_t3", "hit_t4"])
    if [name for _, name in source_labels] != expected_labels:
        raise ValueError(f"unexpected PAM labels: {source_labels}")
    if relative_json == PLANT_JSON:
        commands = data["main_sprite"]["frame"][155]["command"]
        if not any(command[0] == "use_action" for command in commands):
            raise ValueError("attack action point moved from source frame 155")

    pam.EXCLUDED_SPRITE_NAMES = EXCLUDED_SPRITES
    child_states = [pam.simulate_timeline(sprite["frame"])[0] for sprite in data["sprite"]]
    main_states = pam.simulate_timeline(data["main_sprite"]["frame"])
    conversion = pam.Conversion(relative_json, filename, prefix, scale, origin_x, origin_y, {})
    selected = [(name, i) for name, start, end in labels for i in range(start, end)]
    flattened = [pam.expand_frame(data, child_states, main_states[i], conversion)
                 for _, i in selected]
    count = len(flattened)
    tracks = ["<fps>30</fps>"]
    offset = 0
    for name, start, end in labels:
        span = end - start
        tracks.append(pam.marker_track(name, offset, offset + span - 1, count))
        offset += span
    paths = sorted({path for frame in flattened for path in frame})
    for layer in paths:
        tracks.append(pam.visual_track("layer_" + "_".join(map(str, layer)),
                                       [frame.get(layer) for frame in flattened], prefix))

    image_indices = sorted({state["image_index"] for frame in flattened for state in frame.values()})
    assets = [(pam.image_source(source, data["image"][i]), f"{prefix}_{i:03d}.png")
              for i in image_indices]
    return ("\n".join(tracks) + "\n").encode("utf-8"), assets


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if not args.source.is_dir():
        parser.error(f"missing source: {args.source}")
    if not args.check:
        OUTPUT.mkdir(parents=True, exist_ok=True)

    work = [
        # The plant's PAM origin differs from the projectile effect origin.
        # Align the visible feet with the lawn cell and the sprite with its seed card.
        (PLANT_JSON, "PoisonPeashooter.reanim", "POISONPEASHOOTER", PLANT_LABELS, 0.55, 112.0, 110.0),
        (PROJECTILE_JSON, "PoisonPeashooterProjectile.reanim", "POISONPEASHOOTER_PROJECTILE",
         PROJECTILE_LABELS, 0.78, 176.0, 195.0),
    ]
    ok = True
    count = 0
    expected_files = set()
    for relative_json, filename, prefix, labels, scale, origin_x, origin_y in work:
        xml, assets = generate(args.source, relative_json, filename, prefix, labels,
                               scale, origin_x, origin_y)
        expected_files.add(filename)
        ok = pam.compare_or_write(OUTPUT / filename, xml, args.check) and ok
        for image, target in assets:
            expected_files.add(target)
            ok = pam.install_asset(image, OUTPUT / target, args.check) and ok
        count += len(assets)
    if count != 51:
        raise ValueError(f"expected 51 level-one image assets, got {count}")
    if args.check:
        actual_files = {p.name for p in OUTPUT.glob("POISONPEASHOOTER*.png")}
        actual_files |= {p.name for p in OUTPUT.glob("PoisonPeashooter*.reanim")}
        extra = actual_files - expected_files
        if extra:
            print(f"unexpected Poison Peashooter output: {sorted(extra)}", file=sys.stderr)
            ok = False
    if args.check and ok:
        print(f"Poison Peashooter resources match source (2 reanims, {count} PNGs).")
    elif not args.check:
        print(f"Generated Poison Peashooter resources (2 reanims, {count} PNGs).")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
