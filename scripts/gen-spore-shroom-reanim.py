#!/usr/bin/env python3
"""Convert the exported PvZ2 Spore-shroom PAM JSON into PvZ-Portable reanims.

The source export is intentionally kept outside this repository.  Generated XML and
the 82 referenced RGBA sprites are checked in so the game has no runtime dependency
on the extraction workspace.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


REPO_ROOT = Path(__file__).resolve().parents[1]
OUTPUT_DIR = REPO_ROOT / "res" / "main" / "reanim"
DEFAULT_SOURCE = Path("/Users/fiture/Privates/pvz-2-crack/work/sporeshroom")

PLANT_JSON = Path("animations-json/1200/PLANT1/PLANT/SPORESHROOM/SPORESHROOM.json")
PROJECTILE_JSON = Path(
    "animations-json/1200/PLANT1/EFFECTS/SPORESHROOM_PROJECTILE/SPORESHROOM_PROJECTILE.json"
)

SOURCE_HASHES = {
    PLANT_JSON: "abfa5007554ee8e43b7ec2289be1ffbfdf364ec14c10432597409749ec73178d",
    PROJECTILE_JSON: "52f16050adf36f7b8056e78ad55e7d27c5093f1a5d53712fa2119be1bdc74be0",
}

# 国服 PAM 会把已装备的植物装扮一起导出。这三个 sprite 是纸袋和蓝色獠牙，
# 不属于 1 级孢子菇本体；图片仍按计划稳定复制，但生成动画永远不引用它们。
EXCLUDED_SPRITE_NAMES = {"_custom", "custom_01", "custom_02"}


@dataclass(frozen=True)
class Conversion:
    source_json: Path
    output_xml: str
    image_prefix: str
    scale: float
    origin_x: float
    origin_y: float
    labels: dict[str, str]


CONVERSIONS = (
    Conversion(
        PLANT_JSON,
        "SporeShroom.reanim",
        "SPORESHROOM",
        0.65,
        112.0,
        122.0,
        {
            "idle": "anim_idle",
            "idle2": "anim_idle2",
            "attack": "anim_shooting",
            "plantfood": "anim_plantfood",
            "water": "anim_water",
            "grow": "anim_grow",
        },
    ),
    Conversion(
        PROJECTILE_JSON,
        "SporeShroomProjectile.reanim",
        "SPORESHROOM_PROJECTILE",
        0.35,
        195.0,
        195.0,
        {"animation": "anim_fly", "hit": "anim_hit", "hit2": "anim_hit2"},
    ),
)

Matrix = tuple[float, float, float, float, float, float]
IDENTITY: Matrix = (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def matrix_from_pam(values: list[float] | None) -> Matrix:
    if values is None:
        return IDENTITY
    if len(values) == 2:
        return (1.0, 0.0, 0.0, 1.0, values[0], values[1])
    if len(values) == 3:
        angle, x, y = values
        return (math.cos(angle), math.sin(angle), -math.sin(angle), math.cos(angle), x, y)
    if len(values) == 6:
        return tuple(values)  # type: ignore[return-value]
    raise ValueError(f"unsupported PAM transform with {len(values)} values")


def matrix_multiply(left: Matrix, right: Matrix) -> Matrix:
    a, b, c, d, x, y = left
    e, f, g, h, u, v = right
    return (
        a * e + c * f,
        b * e + d * f,
        a * g + c * h,
        b * g + d * h,
        a * u + c * v + x,
        b * u + d * v + y,
    )


def simulate_timeline(frames: list[dict]) -> list[dict[int, dict]]:
    state: dict[int, dict] = {}
    result: list[dict[int, dict]] = []
    for frame in frames:
        for removal in frame.get("remove", []):
            state.pop(removal["index"], None)
        for append in frame.get("append", []):
            if append.get("additive"):
                raise ValueError("additive PAM layers are not supported by the reanim format")
            state[append["index"]] = {
                **append,
                "transform": IDENTITY,
                "color": (1.0, 1.0, 1.0, 1.0),
            }
        for change in frame.get("change", []):
            index = change["index"]
            if index not in state:
                raise ValueError(f"PAM changes missing display-list index {index}")
            if change.get("source_rectangle") is not None:
                raise ValueError("source rectangles are not supported")
            if change.get("sprite_frame_number") is not None:
                raise ValueError("sprite frame seeks are not supported")
            if change.get("transform") is not None:
                state[index]["transform"] = matrix_from_pam(change["transform"])
            if change.get("color") is not None:
                color = tuple(change["color"])
                if color[:3] != (1.0, 1.0, 1.0):
                    raise ValueError("RGB colour animation cannot be represented by reanim")
                state[index]["color"] = color
        result.append({index: dict(instance) for index, instance in state.items()})
    return result


def fmt(value: float) -> str:
    if abs(value) < 0.0000005:
        value = 0.0
    return f"{value:.6f}".rstrip("0").rstrip(".")


def decompose(matrix: Matrix, conversion: Conversion) -> dict[str, float]:
    a, b, c, d, tx, ty = matrix
    scale = conversion.scale
    a *= scale
    b *= scale
    c *= scale
    d *= scale
    scale_x = math.hypot(a, b)
    scale_y = math.hypot(c, d)
    skew_x = math.degrees(math.atan2(b, a))
    skew_y = math.degrees(math.atan2(-c, d))
    return {
        # Reanimator applies its affine transform to image-centred vertices after
        # inserting the half-width/half-height pivot.  Consequently x/y denote
        # the transformed top-left corner, just like PAM's composed matrix.
        "x": (tx - conversion.origin_x) * scale,
        "y": (ty - conversion.origin_y) * scale,
        "kx": skew_x,
        "ky": skew_y,
        "sx": scale_x,
        "sy": scale_y,
    }


def image_source(source_root: Path, image: dict) -> Path:
    short_name = image["name"].split("|", 1)[0]
    matches = list((source_root / "sprites" / "images").rglob(short_name + ".png"))
    if len(matches) != 1:
        raise ValueError(f"expected one PNG for {short_name}, found {len(matches)}")
    return matches[0]


def expand_frame(
    data: dict,
    child_states: list[dict[int, dict]],
    display: dict[int, dict],
    conversion: Conversion,
) -> dict[tuple[int, ...], dict]:
    leaves: dict[tuple[int, ...], dict] = {}

    def expand(instance: dict, path: tuple[int, ...], parent: Matrix, parent_alpha: float) -> None:
        matrix = matrix_multiply(parent, instance["transform"])
        alpha = parent_alpha * instance["color"][3]
        if instance["sprite"]:
            sprite = data["sprite"][instance["resource"]]
            if sprite.get("name") in EXCLUDED_SPRITE_NAMES:
                return
            child = child_states[instance["resource"]]
            for index, nested in sorted(child.items()):
                expand(nested, path + (index,), matrix, alpha)
            return

        image_index = instance["resource"]
        image = data["image"][image_index]
        final_matrix = matrix_multiply(matrix, matrix_from_pam(image["transform"]))
        leaves[path] = {
            **decompose(final_matrix, conversion),
            "alpha": alpha,
            "image_index": image_index,
        }

    for index, instance in sorted(display.items()):
        expand(instance, (index,), IDENTITY, 1.0)
    return leaves


def marker_track(name: str, start: int, end: int, frame_count: int) -> str:
    lines = ["<track>", f"<name>{name}</name>"]
    for frame in range(frame_count):
        visible = start <= frame <= end
        lines.append("<t><f>0</f></t>" if visible else "<t><f>-1</f></t>")
    lines.append("</track>")
    return "\n".join(lines)


def visual_track(name: str, states: list[dict | None], image_prefix: str) -> str:
    lines = ["<track>", f"<name>{name}</name>"]
    for state in states:
        if state is None:
            lines.append("<t><f>-1</f></t>")
            continue
        image_name = f"IMAGE_REANIM_{image_prefix}_{state['image_index']:03d}"
        lines.append(
            "<t>"
            f"<x>{fmt(state['x'])}</x><y>{fmt(state['y'])}</y>"
            f"<kx>{fmt(state['kx'])}</kx><ky>{fmt(state['ky'])}</ky>"
            f"<sx>{fmt(state['sx'])}</sx><sy>{fmt(state['sy'])}</sy>"
            f"<f>0</f><a>{fmt(state['alpha'])}</a><i>{image_name}</i>"
            "</t>"
        )
    lines.append("</track>")
    return "\n".join(lines)


def convert(source_root: Path, conversion: Conversion) -> tuple[bytes, list[tuple[Path, str]]]:
    json_path = source_root / conversion.source_json
    expected_hash = SOURCE_HASHES[conversion.source_json]
    actual_hash = sha256(json_path)
    if actual_hash != expected_hash:
        raise ValueError(
            f"unexpected source JSON hash for {conversion.source_json}: {actual_hash} != {expected_hash}"
        )

    data = json.loads(json_path.read_text(encoding="utf-8"))
    if data["version"] != 6 or data["frame_rate"] != 30:
        raise ValueError("expected PAM version 6 at 30 FPS")
    if any(len(sprite["frame"]) != 1 for sprite in data["sprite"]):
        raise ValueError("this converter expects one-frame child sprites")

    child_states = [simulate_timeline(sprite["frame"])[0] for sprite in data["sprite"]]
    main_states = simulate_timeline(data["main_sprite"]["frame"])
    flattened = [expand_frame(data, child_states, state, conversion) for state in main_states]
    frame_count = len(flattened)

    source_labels = [
        (index, frame["label"])
        for index, frame in enumerate(data["main_sprite"]["frame"])
        if frame.get("label")
    ]
    if [label for _, label in source_labels] != list(conversion.labels):
        raise ValueError(f"unexpected labels in {conversion.source_json}: {source_labels}")

    tracks = ["<fps>30</fps>"]
    for label_index, (start, source_label) in enumerate(source_labels):
        end = source_labels[label_index + 1][0] - 1 if label_index + 1 < len(source_labels) else frame_count - 1
        tracks.append(marker_track(conversion.labels[source_label], start, end, frame_count))

    paths = sorted({path for frame in flattened for path in frame})
    for path in paths:
        states = [frame.get(path) for frame in flattened]
        track_name = "layer_" + "_".join(str(index) for index in path)
        tracks.append(visual_track(track_name, states, conversion.image_prefix))

    xml = ("\n".join(tracks) + "\n").encode("utf-8")
    assets = [
        (image_source(source_root, image), f"{conversion.image_prefix}_{index:03d}.png")
        for index, image in enumerate(data["image"])
    ]
    return xml, assets


def compare_or_write(path: Path, content: bytes, check: bool) -> bool:
    if check:
        if not path.exists() or path.read_bytes() != content:
            print(f"out of date: {path.relative_to(REPO_ROOT)}", file=sys.stderr)
            return False
        return True
    path.write_bytes(content)
    return True


def install_asset(source: Path, target: Path, check: bool) -> bool:
    if check:
        if not target.exists() or sha256(source) != sha256(target):
            print(f"out of date: {target.relative_to(REPO_ROOT)}", file=sys.stderr)
            return False
        return True
    shutil.copyfile(source, target)
    return True


def expected_generated_names() -> set[str]:
    names = {conversion.output_xml for conversion in CONVERSIONS}
    names.update(f"SPORESHROOM_{index:03d}.png" for index in range(75))
    names.update(f"SPORESHROOM_PROJECTILE_{index:03d}.png" for index in range(7))
    return names


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE, help="Spore-shroom export directory")
    parser.add_argument("--check", action="store_true", help="verify committed outputs without modifying them")
    args = parser.parse_args(argv)

    if not args.source.is_dir():
        parser.error(f"source directory does not exist: {args.source}")
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    ok = True
    generated_names: set[str] = set()
    for conversion in CONVERSIONS:
        xml, assets = convert(args.source, conversion)
        xml_path = OUTPUT_DIR / conversion.output_xml
        ok = compare_or_write(xml_path, xml, args.check) and ok
        generated_names.add(xml_path.name)
        for source, name in assets:
            target = OUTPUT_DIR / name
            ok = install_asset(source, target, args.check) and ok
            generated_names.add(name)

    if generated_names != expected_generated_names():
        raise AssertionError("generated file manifest is not the expected 2 XML + 82 PNG files")

    if args.check:
        if ok:
            print("Spore-shroom generated resources are up to date (2 reanims, 82 PNGs).")
        return 0 if ok else 1
    print("Generated Spore-shroom resources (2 reanims, 82 PNGs).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
