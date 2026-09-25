#!/usr/bin/env python3
"""把激光豌豆的枪管原图（500x500）缩放着色成 reanim 需要的 43x27 部件图。

为什么需要这一步
----------------
机枪射手的枪管部件 `res/main/reanim/GatlingPea_barrel.png` 只有 **43x27**：
reanim 是按「帧号 -> 贴图某一列」索引的，所以运行期换贴图的安全阀
（`src/Lawn/Plant.cpp` 的 `ApplyReanimArtSwaps`）要求新图与被替换的原图
**宽高完全一致**，否则整体回退（宁可不好看，也不串帧）。

激光豌豆的枪管原图是 500x500 的带透明通道 PNG，因此这里离线做一次：
  1. 按 alpha 求外接框，裁掉四周空白；
  2. 等比缩放到能放进 43x27（面积平均 / box filter，在 **预乘 alpha** 空间做，
     避免透明像素的黑色把边缘拉脏 —— 直接平均非预乘 RGBA 会出一圈黑边）；
  3. 按 alpha 加权把主体重心落到画布中心，贴进 43x27 透明画布。

原图保持不动，输出另存为 `LaserPea_barrel_small.png`（会被 `pak.py` 一起打进 main.pak）。
观感（枪管落在哪、朝哪、大小是否合适）仍以进游戏为准；本脚本只保证
「尺寸合规 + 带透明通道 + 主体居中」，具体微调见 CONTEXT.md 的记录。

用法：
    python tools/make-laser-pea-barrel.py
    python tools/make-laser-pea-barrel.py --info   # 只看原图的 alpha 外接框与重心，不写文件
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

def _load_png_module():
    """复用 reanim 预览工具里已经写好的纯 Python PNG 读写（只依赖 zlib + struct，不需要 Pillow）。

    文件名带连字符不能直接 import，按路径加载；该模块有 __main__ 保护，导入时不会跑 main()。
    """
    import importlib.util

    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'reanim-preview.py')
    spec = importlib.util.spec_from_file_location('reanim_preview', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


_png = _load_png_module()

# reanim 里被替换的原图尺寸：必须一致
TARGET_WIDTH = 43
TARGET_HEIGHT = 27


def alpha_bbox(width, height, rgba):
    """返回不透明像素的外接框 (x0, y0, x1, y1)（含端点）；整图全透明时返回 None。"""
    x0, y0, x1, y1 = width, height, -1, -1
    for y in range(height):
        row = y * width * 4
        for x in range(width):
            if rgba[row + x * 4 + 3] > 0:
                if x < x0:
                    x0 = x
                if x > x1:
                    x1 = x
                if y < y0:
                    y0 = y
                if y > y1:
                    y1 = y
    if x1 < 0:
        return None
    return (x0, y0, x1, y1)


def row_alpha_profile(width, height, rgba, bbox):
    """每行的 alpha 像素数（只看外接框内），用于把上下两根管子分开。"""
    x0, y0, x1, y1 = bbox
    return [sum(1 for x in range(x0, x1 + 1) if rgba[(y * width + x) * 4 + 3] > 0)
            for y in range(y0, y1 + 1)]


def split_tubes(width, height, rgba, bbox):
    """原图是「上下两根管子 + 中间的连接颈」。

    返回 (上面的管子外接框, 下面的管子外接框)。做法：先按每行 alpha 像素数找连通的两段
    （profile > 0 的连续区间），再在**上半部分**的区间里找"局部最细处"当作上下管的分界线。
    这套几何只针对这一张原图，不追求通用。
    """
    x0, y0, x1, y1 = bbox
    profile = row_alpha_profile(width, height, rgba, bbox)

    # profile > 0 的连续行区间
    spans = []
    start = None
    for i, count in enumerate(profile):
        if count > 0 and start is None:
            start = i
        elif count == 0 and start is not None:
            spans.append((start, i - 1))
            start = None
    if start is not None:
        spans.append((start, len(profile) - 1))

    if len(spans) < 2:
        # 两根管子连在一起（中间没有完全空白的行）：在图像中部找"最细的一行"当分界。
        # 必须在**中部**找：顶部/底部的圆角边缘行天然只有几个像素宽，拿它们当分界线会把整根管子切掉。
        # 先做一点平滑（3 行滑动平均），免得噪声行当选。
        smoothed = [sum(profile[max(0, i - 1):i + 2]) / len(profile[max(0, i - 1):i + 2])
                    for i in range(len(profile))]
        lo = int(len(profile) * 0.25)
        hi = max(lo + 1, int(len(profile) * 0.75))
        waist = min(range(lo, hi), key=lambda i: smoothed[i])
        spans = [(spans[0][0], waist), (waist + 1, spans[0][1])]

    return ((x0, y0 + spans[0][0], x1, y0 + spans[0][1]),
            (x0, y0 + spans[1][0], x1, y0 + spans[1][1]))


def alpha_centroid(width, height, rgba, bbox):
    """alpha 加权的重心（用于居中）。"""
    x0, y0, x1, y1 = bbox
    total = 0.0
    sum_x = 0.0
    sum_y = 0.0
    for y in range(y0, y1 + 1):
        row = y * width * 4
        for x in range(x0, x1 + 1):
            alpha = rgba[row + x * 4 + 3]
            if alpha == 0:
                continue
            total += alpha
            sum_x += alpha * (x + 0.5)
            sum_y += alpha * (y + 0.5)
    if total <= 0.0:
        return ((x0 + x1 + 1) * 0.5, (y0 + y1 + 1) * 0.5)
    return (sum_x / total, sum_y / total)


def box_downsample(width, height, rgba, bbox, out_width, out_height):
    """按外接框裁切并等比缩放到 (out_width, out_height)，面积平均，预乘 alpha。"""
    x0, y0, x1, y1 = bbox
    crop_w = x1 - x0 + 1
    crop_h = y1 - y0 + 1

    scale = min(out_width / crop_w, out_height / crop_h)
    draw_w = max(1, int(round(crop_w * scale)))
    draw_h = max(1, int(round(crop_h * scale)))
    offset_x = (out_width - draw_w) // 2
    offset_y = (out_height - draw_h) // 2

    out = bytearray(out_width * out_height * 4)   # 全透明
    for dy in range(draw_h):
        src_y0 = y0 + int(dy * crop_h / draw_h)
        src_y1 = y0 + int((dy + 1) * crop_h / draw_h)
        if src_y1 <= src_y0:
            src_y1 = src_y0 + 1
        src_y1 = min(src_y1, y1 + 1)

        for dx in range(draw_w):
            src_x0 = x0 + int(dx * crop_w / draw_w)
            src_x1 = x0 + int((dx + 1) * crop_w / draw_w)
            if src_x1 <= src_x0:
                src_x1 = src_x0 + 1
            src_x1 = min(src_x1, x1 + 1)

            # 预乘空间求平均：acc[3] 是 alpha 之和，acc[0..2] 是 premultiplied 之和
            acc = [0.0, 0.0, 0.0, 0.0]
            count = 0
            for sy in range(src_y0, src_y1):
                row = sy * width * 4
                for sx in range(src_x0, src_x1):
                    index = row + sx * 4
                    alpha = rgba[index + 3] / 255.0
                    acc[0] += rgba[index + 0] * alpha
                    acc[1] += rgba[index + 1] * alpha
                    acc[2] += rgba[index + 2] * alpha
                    acc[3] += alpha
                    count += 1

            if count == 0 or acc[3] <= 0.0:
                continue

            target = ((offset_y + dy) * out_width + (offset_x + dx)) * 4
            out[target + 0] = max(0, min(255, int(round(acc[0] / acc[3]))))
            out[target + 1] = max(0, min(255, int(round(acc[1] / acc[3]))))
            out[target + 2] = max(0, min(255, int(round(acc[2] / acc[3]))))
            out[target + 3] = max(0, min(255, int(round(acc[3] / count * 255.0))))

    return out, (offset_x, offset_y, draw_w, draw_h)


def recenter(out_width, out_height, rgba, target_cx, target_cy):
    """把整张画布按整数像素平移，使 alpha 重心尽量落在 (target_cx, target_cy)。"""
    bbox = alpha_bbox(out_width, out_height, rgba)
    if bbox is None:
        return rgba, (0, 0)
    cx, cy = alpha_centroid(out_width, out_height, rgba, bbox)
    shift_x = int(round(target_cx - cx))
    shift_y = int(round(target_cy - cy))
    if shift_x == 0 and shift_y == 0:
        return rgba, (0, 0)

    shifted = bytearray(out_width * out_height * 4)
    for y in range(out_height):
        src_y = y - shift_y
        if src_y < 0 or src_y >= out_height:
            continue
        for x in range(out_width):
            src_x = x - shift_x
            if src_x < 0 or src_x >= out_width:
                continue
            src = (src_y * out_width + src_x) * 4
            dst = (y * out_width + x) * 4
            shifted[dst:dst + 4] = rgba[src:src + 4]
    return shifted, (shift_x, shift_y)


def main():
    parser = argparse.ArgumentParser(description='生成 43x27 的激光豌豆枪管部件图')
    parser.add_argument('--source', default=os.path.join('res', 'main', 'reanim', 'LaserPea_barrel.png'),
                        help='枪管原图（默认 res/main/reanim/LaserPea_barrel.png）')
    parser.add_argument('--output', default=os.path.join('res', 'main', 'reanim', 'LaserPea_barrel_small.png'),
                        help='输出部件图（默认 res/main/reanim/LaserPea_barrel_small.png）')
    parser.add_argument('--part', choices=('upper', 'lower'), default='lower',
                        help='原图是上下两根管子叠在一起，只保留哪一根（默认 lower = 较长的那根主枪管）')
    parser.add_argument('--info', action='store_true', help='只打印原图的 alpha 外接框/重心，不写文件')
    args = parser.parse_args()

    if not os.path.exists(args.source):
        raise SystemExit('找不到原图：%s' % args.source)

    width, height, rgba = _png.load_png(args.source)
    bbox = alpha_bbox(width, height, rgba)
    if bbox is None:
        raise SystemExit('原图完全没有不透明像素：%s' % args.source)
    upper_bbox, lower_bbox = split_tubes(width, height, rgba, bbox)
    picked = upper_bbox if args.part == 'upper' else lower_bbox
    cx, cy = alpha_centroid(width, height, rgba, bbox)
    print('source %s: %dx%d' % (args.source, width, height))
    print('  alpha bbox   = %s (w=%d h=%d)' % (bbox, bbox[2] - bbox[0] + 1, bbox[3] - bbox[1] + 1))
    print('  alpha center = (%.2f, %.2f)' % (cx, cy))
    print('  upper tube   = %s (h=%d)' % (upper_bbox, upper_bbox[3] - upper_bbox[1] + 1))
    print('  lower tube   = %s (h=%d)' % (lower_bbox, lower_bbox[3] - lower_bbox[1] + 1))
    print('  picked       = %s' % (args.part,))
    if args.info:
        return 0

    scaled, layout = box_downsample(width, height, rgba, picked, TARGET_WIDTH, TARGET_HEIGHT)
    scaled, shift = recenter(TARGET_WIDTH, TARGET_HEIGHT, scaled, TARGET_WIDTH / 2.0, TARGET_HEIGHT / 2.0)
    _png.write_png(args.output, TARGET_WIDTH, TARGET_HEIGHT, bytes(scaled))

    out_bbox = alpha_bbox(TARGET_WIDTH, TARGET_HEIGHT, scaled)
    out_cx, out_cy = alpha_centroid(TARGET_WIDTH, TARGET_HEIGHT, scaled, out_bbox)
    opaque = sum(1 for i in range(TARGET_WIDTH * TARGET_HEIGHT) if scaled[i * 4 + 3] > 0)
    print('wrote %s: %dx%d' % (args.output, TARGET_WIDTH, TARGET_HEIGHT))
    print('  fit          = %dx%d at (%d, %d)' % (layout[2], layout[3], layout[0], layout[1]))
    print('  recenter     = (%d, %d)' % shift)
    print('  alpha bbox   = %s' % (out_bbox,))
    print('  alpha center = (%.2f, %.2f)  (target %.1f, %.1f)'
          % (out_cx, out_cy, TARGET_WIDTH / 2.0, TARGET_HEIGHT / 2.0))
    print('  opaque px    = %d / %d' % (opaque, TARGET_WIDTH * TARGET_HEIGHT))
    if opaque == 0:
        raise SystemExit('输出全透明，说明缩放参数有问题')
    return 0


if __name__ == '__main__':
    sys.exit(main())
