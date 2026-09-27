#!/usr/bin/env python3
"""Build GatlingCactus.reanim from the stock cactus motion tracks.

The extra tracks follow the cactus face and mouth in every frame, including
rise and lower. Existing Gatling Pea images supply the helmet and barrels.

The stock Cactus_lips track stays exactly as it is in Cactus.reanim (it is the
cactus mouth's base art, drawn behind the gun). GatlingCactus_lips_overlay adds
a second, trimmed copy of the same lip on top of the barrels, the way the stock
Gatling Pea draws GatlingPea_mouth_overlay over its own mouth: the near lip has
to stay in front of the barrel base, and the barrel recoil must not eat it.

The gun itself is anchored at the lip, offset by (GUN_OFFSET_X, GUN_OFFSET_Y).
The older GatlingCactus_mouth_overlay cuff is no longer emitted.
"""

import argparse
from pathlib import Path
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "res/main/reanim/Cactus.reanim"
GATLING_SOURCE = ROOT / "res/main/reanim/GatlingPea.reanim"
TARGET = ROOT / "res/main/reanim/GatlingCactus.reanim"
FLOWER_TRACKS = {"Cactus_flower", "Cactus_floer_top", "Cactus_floer_top2", "Cactus_floer_top3"}
# A second copy of the stock lip, drawn after the barrels (see the module
# docstring). Cactus_lips itself is copied from the source untouched.
LIPS_OVERLAY_TRACK = "GatlingCactus_lips_overlay"
LIPS_OVERLAY_IMAGE = "IMAGE_REANIM_GATLINGCACTUS_LIPS_OVERLAY"
# The four barrels keep the stock Gatling Pea's depth order and its relative
# centers. ThreeGaling uses roughly half of Gatling Pea's 0.55 barrel scale.
BARRELS = (("3", -4.0, -5.0), ("4", -5.0, -2.0), ("2", -5.0, -8.0), ("1", -6.5, -5.0))
BARREL_SCALE = 0.271
# The cactus lip track sits above and behind its visible mouth. Position the
# gun on the mouth opening: the barrels then poke out past the lip's right edge
# while the lip's near half covers their base.
GUN_OFFSET_X = 5.0
GUN_OFFSET_Y = 10.0


def barrel_motion_frame(index):
    # Keep the barrels gently turning at the cactus's own idle frame rate.
    # Shooting then uses the same opening frames at the stock shooting rate.
    if 5 <= index < 20:
        return 54 + (index - 5) * 38 / 14
    if 20 <= index < 38:
        return 54 + index - 20
    if 53 <= index < 68:
        return 54 + (index - 53) * 38 / 14
    if 68 <= index < 82:
        return 54 + index - 68
    return None


def sample_motion(frames, frame):
    before = int(frame)
    fraction = frame - before
    if fraction == 0:
        return frames[before]
    return {key: value + (frames[before + 1][key] - value) * fraction
            for key, value in frames[before].items()}


def resolved_frames(track):
    value = {key: 0.0 for key in ("x", "y", "kx", "ky")}
    value.update(sx=1.0, sy=1.0, f=-1.0)
    frames = []
    for frame in track.findall("t"):
        value = value.copy()
        for item in frame:
            if item.tag in value:
                value[item.tag] = float(item.text)
        frames.append(value)
    return frames


def number(value):
    return f"{value:.3f}".rstrip("0").rstrip(".")


def new_track(name, image, positions):
    lines = ["<track>", f"<name>{name}</name>"]
    for index, (x, y, sx, sy, kx, ky) in enumerate(positions):
        if index == 0:
            lines.append("<t><f>-1</f></t>")
        elif index < 5:
            lines.append("<t></t>")
        else:
            fields = {"x": x, "y": y, "kx": kx, "ky": ky, "sx": sx, "sy": sy}
            content = "".join(f"<{key}>{number(value)}</{key}>" for key, value in fields.items())
            if index == 5:
                content += f"<f>0</f><i>{image}</i>"
            lines.append(f"<t>{content}</t>")
    return "\n".join(lines + ["</track>"])


def generate():
    source = SOURCE.read_text(encoding="utf-8")
    root = ET.fromstring("<root>" + source + "</root>")
    tracks = {track.findtext("name"): resolved_frames(track) for track in root.findall("track")}
    face, lips = tracks["anim_face"], tracks["Cactus_lips"]
    assert len(face) == len(lips) == 95
    gatling = ET.fromstring("<root>" + GATLING_SOURCE.read_text(encoding="utf-8") + "</root>")
    gatling_tracks = {track.findtext("name"): resolved_frames(track) for track in gatling.findall("track")}

    parts = []
    for part in source.split("</track>"):
        if not part.strip():
            continue
        if any(f"<name>{name}</name>" in part for name in FLOWER_TRACKS):
            part = part.replace("<f>0</f>", "<f>-1</f>")
        parts.append(part + "</track>")
    result = "".join(parts).rstrip() + "\n"

    for segment, dx, dy in BARRELS:
        positions = []
        original = gatling_tracks[f"GatlingPea_barrel{segment}"]
        rest = original[54]
        for index, (head, lip) in enumerate(zip(face, lips)):
            phase = barrel_motion_frame(index)
            motion = sample_motion(original, phase) if phase is not None else rest
            # Reuse each original barrel's recoil, roll and tilt, scaled to the
            # ThreeGaling-sized assembly. The cactus lip remains the anchor as
            # its head stretches during shooting and growth.
            x = lip["x"] + GUN_OFFSET_X + dx + (motion["x"] - rest["x"]) * 0.5
            y = lip["y"] + GUN_OFFSET_Y + dy + (motion["y"] - rest["y"]) * 0.5
            scale_x = BARREL_SCALE * head["sx"] / 0.8 * motion["sx"] / rest["sx"]
            scale_y = BARREL_SCALE * head["sy"] / 0.8 * motion["sy"] / rest["sy"]
            positions.append((x, y, scale_x, scale_y,
                              lip["kx"] + motion["kx"] - rest["kx"],
                              lip["ky"] + motion["ky"] - rest["ky"]))
        result += new_track(f"GatlingCactus_barrel{segment}", "IMAGE_REANIM_GATLINGPEA_BARREL", positions) + "\n"

    # The same lip again, this time on top of the barrels (the layer slot the
    # stock Gatling Pea gives its own mouth overlay). It reuses the stock lip's
    # per-frame position, scale and tilt; the trimmed art covers the barrels'
    # base only, so the gun reads as coming out from behind the near lip and the
    # recoil can never eat that edge.
    positions = []
    for lip in lips:
        positions.append((lip["x"], lip["y"], lip["sx"], lip["sy"],
                          lip["kx"], lip["ky"]))
    result += new_track(LIPS_OVERLAY_TRACK, LIPS_OVERLAY_IMAGE, positions) + "\n"

    # The old painted "lip cuff" (GatlingCactus_mouth_overlay, a 40x80 crescent
    # at 0.4 scale) is deliberately not emitted any more: it read as a stray
    # green hook hanging below the gun. res/main/reanim keeps the PNG in case the
    # art is wanted again.
    positions = []
    for head in face:
        positions.append((head["x"] - 7.0, head["y"] - 12.0,
                          0.58 * head["sx"] / 0.8, 0.58 * head["sy"] / 0.8,
                          head["kx"], head["ky"]))
    result += new_track("GatlingCactus_helmet", "IMAGE_REANIM_GATLINGPEA_HELMET", positions) + "\n"
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="verify the committed reanim without writing")
    args = parser.parse_args()
    expected = generate()
    if args.check:
        if not TARGET.exists() or TARGET.read_text(encoding="utf-8") != expected:
            parser.error("GatlingCactus.reanim is missing or out of date")
        print("GatlingCactus.reanim is up to date")
    else:
        TARGET.write_text(expected, encoding="utf-8")
        print(f"Wrote {TARGET}")


if __name__ == "__main__":
    main()
