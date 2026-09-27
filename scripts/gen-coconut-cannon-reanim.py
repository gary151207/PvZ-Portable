#!/usr/bin/env python3
"""Generate level-one Coconut Cannon animations from the supplied PvZ2 PAM export."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = Path("/Users/fiture/Privates/pvz-2-crack/work/coconutcannon")
OUTPUT = ROOT / "res/main/reanim"
spec = importlib.util.spec_from_file_location("pvz2_pam_converter", Path(__file__).with_name("gen-spore-shroom-reanim.py"))
assert spec and spec.loader
pam = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = pam
spec.loader.exec_module(pam)

PLANT = Path("animations-json/1200/PLANT1/PLANT/COCONUTCANNON/COCONUTCANNON.json")
PROJECTILE = Path("animations-json/1200/PLANT1/EFFECTS/COCONUT_PROJECTILE_EXPLOSION/COCONUT_PROJECTILE_EXPLOSION.json")
HASHES = {
    PLANT: "21b85850a48074aca6558402dfa34c6764a9e69977d243bf65bc8babd5dfe77c",
    PROJECTILE: "79a871b05d24df54a5755a439a189da8a6b4546ac3db08ed6b02bc156f588e21",
}
WORK = (
    (PLANT, "CoconutCannon.reanim", "COCONUTCANNON",
     (("anim_idle", 0, 30), ("anim_idle2", 30, 60), ("anim_idle3", 60, 121),
      ("anim_shooting", 121, 161), ("anim_recover", 211, 262)), 0.45, 90.0, 75.0, 30),
    (PROJECTILE, "CoconutCannonProjectile.reanim", "COCONUT_PROJECTILE",
     (("anim_fly", 0, 5), ("anim_hit", 35, 85)), 0.8, 170.0, 147.0, 24),
)


def generate(source: Path, relative: Path, filename: str, prefix: str,
             spans: tuple[tuple[str, int, int], ...], scale: float,
             origin_x: float, origin_y: float, fps: int) -> tuple[bytes, list[tuple[Path, str]]]:
    path = source / relative
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if digest != HASHES[relative]:
        raise ValueError(f"unexpected source hash: {relative}: {digest}")
    data = json.loads(path.read_text(encoding="utf-8"))
    if data["version"] != 6 or data["frame_rate"] != fps:
        raise ValueError(f"expected PAM version 6 at {fps} FPS")
    if any(len(sprite["frame"]) != 1 for sprite in data["sprite"]):
        raise ValueError("unexpected multi-frame child sprite")
    labels = [(i, f["label"]) for i, f in enumerate(data["main_sprite"]["frame"]) if f.get("label")]
    expected = (["idle", "idle2", "idle3", "attack", "plantfood", "recover", "recover2", "water"]
                if relative == PLANT else
                ["coconut_projectile", "coconut_projectile_plantfood", "coconut_projectile_plantfood_fuse",
                 "coconut_explosion", "coconut_projectile_plantfood_fuse2", "coconut_projectile_plantfood2"])
    if [name for _, name in labels] != expected:
        raise ValueError(f"unexpected labels: {labels}")
    if relative == PLANT and not any(c[0] == "use_action" for c in data["main_sprite"]["frame"][133].get("command", [])):
        raise ValueError("normal attack action point moved from frame 133")

    # Some source layers carry a grayscale color multiplier. The reanim format
    # only represents per-frame alpha; retain geometry and opacity here.
    for frames in [data["main_sprite"]["frame"], *(sprite["frame"] for sprite in data["sprite"])]:
        for frame in frames:
            for change in frame.get("change", []):
                color = change.get("color")
                if color is not None:
                    if color[0] != color[1] or color[1] != color[2]:
                        raise ValueError("unsupported non-grayscale RGB animation")
                    change["color"] = [1.0, 1.0, 1.0, color[3]]

    pam.EXCLUDED_SPRITE_NAMES = {"custom_03", "custom_02", "custom_01", "_custom"}
    children = [pam.simulate_timeline(sprite["frame"])[0] for sprite in data["sprite"]]
    main = pam.simulate_timeline(data["main_sprite"]["frame"])
    flattened = []
    for label, start, end in spans:
        # The source projectile and explosion timelines have different pivots.
        x = origin_x + (5.0 if relative == PROJECTILE and label == "anim_fly" else 0.0)
        y = origin_y + (18.0 if relative == PROJECTILE and label == "anim_fly" else 0.0)
        conversion = pam.Conversion(relative, filename, prefix, scale, x, y, {})
        flattened.extend(pam.expand_frame(data, children, main[i], conversion) for i in range(start, end))
    count = len(flattened)
    tracks = [f"<fps>{fps}</fps>"]
    offset = 0
    for label, start, end in spans:
        length = end - start
        tracks.append(pam.marker_track(label, offset, offset + length - 1, count))
        offset += length
    for layer in sorted({layer for frame in flattened for layer in frame}):
        tracks.append(pam.visual_track("layer_" + "_".join(map(str, layer)),
                                       [frame.get(layer) for frame in flattened], prefix))

    images = sorted({state["image_index"] for frame in flattened for state in frame.values()})
    assets = []
    for i in images:
        name = data["image"][i]["name"].split("|", 1)[0].removeprefix("add$")
        matches = list((source / "sprites/images").rglob(name + ".png"))
        if len(matches) != 1:
            raise ValueError(f"expected one source image for {name}, found {len(matches)}")
        assets.append((matches[0], f"{prefix}_{i:03d}.png"))
    return ("\n".join(tracks) + "\n").encode(), assets


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if not args.source.is_dir():
        parser.error(f"missing source: {args.source}")
    if not args.check:
        OUTPUT.mkdir(parents=True, exist_ok=True)
    ok = True
    expected = set()
    for relative, filename, prefix, spans, scale, ox, oy, fps in WORK:
        xml, assets = generate(args.source, relative, filename, prefix, spans, scale, ox, oy, fps)
        expected.add(filename)
        ok = pam.compare_or_write(OUTPUT / filename, xml, args.check) and ok
        for image, target in assets:
            expected.add(target)
            ok = pam.install_asset(image, OUTPUT / target, args.check) and ok
    actual = {p.name for p in OUTPUT.glob("COCONUTCANNON_*.png")}
    actual |= {p.name for p in OUTPUT.glob("COCONUT_PROJECTILE_*.png")}
    actual |= {p.name for p in OUTPUT.glob("CoconutCannon*.reanim")}
    if args.check and actual != expected:
        print(f"unexpected Coconut Cannon output: {sorted(actual - expected)}", file=sys.stderr)
        ok = False
    if ok:
        print(f"Coconut Cannon resources {'match source' if args.check else 'generated'}: {len(expected) - 2} PNGs, 2 reanims")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
