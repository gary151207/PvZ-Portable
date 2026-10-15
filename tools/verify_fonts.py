"""Verify the extracted fonts load and render, and build a preview sheet."""
from __future__ import annotations

import glob
import os
import struct

from PIL import Image, ImageDraw, ImageFont

WS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONT_DIR = os.path.join(WS, "extracted-fonts")
OUT = os.path.join(FONT_DIR, "preview.png")


def ttf_info(path: str) -> dict:
    with open(path, "rb") as fh:
        data = fh.read(12)
    sfnt, num_tables = struct.unpack(">IH", data[:6])
    return {
        "sfnt_version": hex(sfnt),
        "num_tables": num_tables,
    }


def main() -> int:
    files = sorted(glob.glob(os.path.join(FONT_DIR, "*.ttf"))) + sorted(
        glob.glob(os.path.join(FONT_DIR, "*.otf"))
    )
    rows = []
    print(f"{'file':<28} {'size':>9}  {'family':<24} {'style':<10} tables  first-glyph")
    for path in files:
        name = os.path.basename(path)
        size = os.path.getsize(path)
        info = ttf_info(path)
        try:
            font = ImageFont.truetype(path, 40)
            family, style = font.getname()
            ok = "ok"
        except Exception as exc:  # noqa: BLE001
            family = style = f"<{exc}>"
            font = None
            ok = "FAIL"
        print(
            f"{name:<28} {size:>9}  {family:<24} {style:<10} "
            f"{info['num_tables']:>6}  {ok}"
        )
        rows.append((name, family, style, font, size))

    # preview sheet
    pad, line_h, width = 16, 74, 1100
    img = Image.new("RGB", (width, pad + line_h * len(rows) + pad), "white")
    draw = ImageDraw.Draw(img)
    label_font = ImageFont.load_default(size=16)
    y = pad
    for name, family, style, font, size in rows:
        draw.text((pad, y + 4), f"{name}  |  {family} {style}  |  {size:,} bytes",
                  fill="black", font=label_font)
        if font is not None:
            draw.text((pad, y + 28), "AaBb 0123 植物大战僵尸 融合版", fill="black", font=font)
        y += line_h
    img.save(OUT)
    print("preview written:", OUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
