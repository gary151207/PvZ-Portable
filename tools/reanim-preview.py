#!/usr/bin/env python3
"""离线预览 reanim 的一帧：把「引擎会画成什么样」渲染成 PNG（美术调参用）。

用途：给 reanim 加轨道 / 改贴图 / 调头盔与枪管的落位时，不必每次都重新打包 pak + 启动游戏 ——
直接渲染出来看。例如三线机枪射手（res/main/reanim/ThreeGaling.reanim）：

    # 只看 head1 的 idle 起始帧（脸 / 嘴 / 枪管 / 嘴覆层 / 头盔都在这一帧里）
    python tools/reanim-preview.py res/main/reanim/ThreeGaling.reanim head1.png 4 4
    # 整株：身体层 124 + 三个头 4 / 45 / 86（按顺序叠加），放大 4 倍
    python tools/reanim-preview.py res/main/reanim/ThreeGaling.reanim plant.png 124,4,45,86 4

参数：<reanim 文件> <输出 PNG> <帧号[,帧号...]> <放大倍数>

它只实现核对落位/层次所需的那部分引擎语义：

  * 逐帧"缺省字段沿用上一帧"（ReanimationLoadDefinition 的 ReanimationFillInMissingData）；
  * 轨道变换矩阵（Reanimation::MatrixFromTransform：m00/m10 = Rot(-kx)·sx、m01/m11 = Rot(-ky)·sy，
    平移就是 x/y；图像左上角落在 (x, y)，与引擎一致）；
  * 按**轨道在文件里的顺序**做 source-over 合成，`<f>` 为负的空白帧跳过
    （这正是"轨道顺序 = 绘制顺序"那条规则）。

**近似之处**（只用于看位置与层次，不是像素级还原）：最近邻采样、不做线性混合；
不计算 attach 的父子矩阵（所以头实例按它自己轨道里的坐标直接画 —— 对那些本来就用植物坐标
书写的头轨道（如 ThreePeater / ThreeGaling 的 anim_faceN）而言结果一致）。
"""

import argparse
import math
import struct
import sys
import zlib
import xml.etree.ElementTree as ET

REANIM_DIR = 'res/main/reanim/'
FIELDS = ('x', 'y', 'kx', 'ky', 'sx', 'sy', 'f', 'a')

_png_cache = {}


def load_png(path):
    if path in _png_cache:
        return _png_cache[path]
    data = open(path, 'rb').read()
    pos = 8
    idat = b''
    palette = b''
    transparency = b''
    width = height = depth = color = None
    while pos < len(data):
        length = struct.unpack('>I', data[pos:pos + 4])[0]
        kind = data[pos + 4:pos + 8]
        chunk = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b'IHDR':
            width, height, depth, color, comp, filt, interlace = struct.unpack('>IIBBBBB', chunk)
            if depth != 8 or interlace != 0:
                raise SystemExit('only 8-bit non-interlaced PNGs are supported: %s' % path)
        elif kind == b'PLTE':
            palette = chunk
        elif kind == b'tRNS':
            transparency = chunk
        elif kind == b'IDAT':
            idat += chunk
        elif kind == b'IEND':
            break

    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[color]
    raw = zlib.decompress(idat)
    stride = width * channels
    out = bytearray()
    previous = bytearray(stride)
    cursor = 0
    for _ in range(height):
        filter_type = raw[cursor]
        cursor += 1
        line = bytearray(raw[cursor:cursor + stride])
        cursor += stride
        if filter_type == 1:
            for x in range(channels, stride):
                line[x] = (line[x] + line[x - channels]) & 255
        elif filter_type == 2:
            for x in range(stride):
                line[x] = (line[x] + previous[x]) & 255
        elif filter_type == 3:
            for x in range(stride):
                left = line[x - channels] if x >= channels else 0
                line[x] = (line[x] + ((left + previous[x]) >> 1)) & 255
        elif filter_type == 4:
            for x in range(stride):
                left = line[x - channels] if x >= channels else 0
                up = previous[x]
                upleft = previous[x - channels] if x >= channels else 0
                estimate = left + up - upleft
                dl, du, dul = abs(estimate - left), abs(estimate - up), abs(estimate - upleft)
                predictor = left if (dl <= du and dl <= dul) else (up if du <= dul else upleft)
                line[x] = (line[x] + predictor) & 255
        out += line
        previous = line

    rgba = bytearray(width * height * 4)
    for i in range(width * height):
        if channels == 4:
            rgba[i * 4:i * 4 + 4] = out[i * 4:i * 4 + 4]
        elif channels == 3:
            rgba[i * 4:i * 4 + 3] = out[i * 3:i * 3 + 3]
            rgba[i * 4 + 3] = 255
        elif color == 3:
            index = out[i]
            rgba[i * 4:i * 4 + 3] = palette[index * 3:index * 3 + 3]
            rgba[i * 4 + 3] = transparency[index] if index < len(transparency) else 255
        else:
            grey = out[i]
            rgba[i * 4:i * 4 + 3] = bytes((grey, grey, grey))
            rgba[i * 4 + 3] = 255

    result = (width, height, bytes(rgba))
    _png_cache[path] = result
    return result


def write_png(path, width, height, rgba):
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        raw += rgba[y * width * 4:(y + 1) * width * 4]

    def chunk(kind, payload):
        return (struct.pack('>I', len(payload)) + kind + payload
                + struct.pack('>I', zlib.crc32(kind + payload) & 0xFFFFFFFF))

    header = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    with open(path, 'wb') as handle:
        handle.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', header)
                     + chunk(b'IDAT', zlib.compress(bytes(raw), 6)) + chunk(b'IEND', b''))


def load_reanim(path):
    """解析成 [(轨道名, [(每帧有效值, 该帧的贴图名), ...]), ...]（顺序 = 绘制顺序）。"""
    text = open(path, 'r', encoding='utf-8', newline='').read()
    root = ET.fromstring('<root>' + text + '</root>')
    tracks = []
    for track in root.findall('track'):
        name = track.find('name').text
        previous = {'x': 0.0, 'y': 0.0, 'kx': 0.0, 'ky': 0.0, 'sx': 1.0, 'sy': 1.0, 'f': 0.0, 'a': 1.0}
        image = None
        frames = []
        for node in track.findall('t'):
            kids = {child.tag: child.text for child in node}
            for field in FIELDS:
                if field in kids:
                    previous[field] = float(kids[field])
            if 'i' in kids:
                image = kids['i']
            frames.append((dict(previous), image))
        tracks.append((name, frames))
    return tracks


def image_path(name):
    if not name.startswith('IMAGE_REANIM_'):
        raise SystemExit('unsupported image name: %s' % name)
    return REANIM_DIR + name[len('IMAGE_REANIM_'):] + '.png'


def draw_frame(canvas, canvas_w, canvas_h, tracks, index, scale, origin):
    for _, frames in tracks:
        transform, image_name = frames[index]
        if transform['f'] < 0.0 or image_name is None:
            continue
        image_w, image_h, pixels = load_png(image_path(image_name))

        kx = math.radians(transform['kx'])
        ky = math.radians(transform['ky'])
        m00 = math.cos(-kx) * transform['sx'] * scale
        m10 = -math.sin(-kx) * transform['sx'] * scale
        m01 = math.sin(-ky) * transform['sy'] * scale
        m11 = math.cos(-ky) * transform['sy'] * scale
        tx = transform['x'] * scale + origin
        ty = transform['y'] * scale + origin

        corners = [(m00 * px + m01 * py + tx, m10 * px + m11 * py + ty)
                   for (px, py) in ((0, 0), (image_w, 0), (0, image_h), (image_w, image_h))]
        xs = [corner[0] for corner in corners]
        ys = [corner[1] for corner in corners]
        x0 = max(0, int(math.floor(min(xs))))
        x1 = min(canvas_w - 1, int(math.ceil(max(xs))))
        y0 = max(0, int(math.floor(min(ys))))
        y1 = min(canvas_h - 1, int(math.ceil(max(ys))))

        determinant = m00 * m11 - m01 * m10
        if abs(determinant) < 1e-9:
            continue
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                dx = x + 0.5 - tx
                dy = y + 0.5 - ty
                sx = (m11 * dx - m01 * dy) / determinant
                sy = (-m10 * dx + m00 * dy) / determinant
                if sx < 0 or sy < 0 or sx >= image_w or sy >= image_h:
                    continue
                source = (int(sy) * image_w + int(sx)) * 4
                alpha = pixels[source + 3]
                if alpha == 0:
                    continue
                target = (y * canvas_w + x) * 4
                weight = alpha / 255.0
                for channel in range(3):
                    canvas[target + channel] = int(pixels[source + channel] * weight
                                                   + canvas[target + channel] * (1 - weight))
                canvas[target + 3] = max(canvas[target + 3], alpha)


def main():
    parser = argparse.ArgumentParser(description='把 reanim 的某一帧渲染成 PNG（美术调参用）')
    parser.add_argument('reanim', help='reanim 文件（例如 res/main/reanim/ThreeGaling.reanim）')
    parser.add_argument('output', help='输出 PNG')
    parser.add_argument('frames', help='帧号，逗号分隔；按先后顺序叠加（如 124,4,45,86 = 身体 + 三个头）')
    parser.add_argument('scale', nargs='?', type=float, default=4.0, help='放大倍数（默认 4）')
    args = parser.parse_args()

    tracks = load_reanim(args.reanim)
    indices = [int(value) for value in args.frames.split(',')]
    frame_count = len(tracks[0][1])
    for index in indices:
        if index < 0 or index >= frame_count:
            raise SystemExit('帧号 %d 超出范围（这个 reanim 共 %d 帧）' % (index, frame_count))

    margin = 45.0
    canvas_w = int((150 + 2 * margin) * args.scale)
    canvas_h = int((170 + 2 * margin) * args.scale)
    canvas = bytearray(canvas_w * canvas_h * 4)
    for i in range(canvas_w * canvas_h):
        canvas[i * 4 + 0] = 32          # 深绿背景，方便看浅色枪管与透明边缘
        canvas[i * 4 + 1] = 96
        canvas[i * 4 + 2] = 32
        canvas[i * 4 + 3] = 255

    for index in indices:
        draw_frame(canvas, canvas_w, canvas_h, tracks, index, args.scale, margin * args.scale)

    write_png(args.output, canvas_w, canvas_h, bytes(canvas))
    print('wrote %s (%dx%d, frames %s)' % (args.output, canvas_w, canvas_h, indices))


if __name__ == '__main__':
    sys.exit(main())
