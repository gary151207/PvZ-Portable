#!/usr/bin/env python3
"""生成 res/main/reanim/WindPeashooter.reanim（风神豌豆射手的 reanim）。

风神豌豆射手（SEED_WIND_PEASHOOTER）= 普通豌豆射手 + 三叶草的叶子。所以：

  * 骨架直接**复制** reanim/PeaShooterSingle.reanim（104 帧 × 17 条轨道）——
    那正是 REANIM_PEASHOOTER 用的文件，也就是游戏里"普通豌豆射手"的样子；
    待机、开火、成长、眨眼、叶/茎全部免费继承；
  * 追加三条**三叶草叶片**轨道 `WindPeashooter_clover1/2/3`，引用原版
    IMAGE_REANIM_BLOVER_PETAL（三叶草 = 三叶草风扇那张叶子，33x34）。
    三片叶子的相对布局**照抄** Blover.reanim 的 Blover_petals / Blover_petals2 /
    Blover_petals3 在第 0 帧的几何（旋转 0° / 240° / 120°，正好围成一株三叶草），
    再整体等比缩放、旋转并挂到豌豆射手自己那支小叶子（`anim_sprout`）上 ——
    位置与倾角逐帧跟随 sprout，所以三叶草跟着植株一起摇，
    而且**每个动作段内部都是无缝循环**（不依赖 Blover 的动画相位）。

绘制顺序（= 本文件里的轨道顺序）：`anim_stem → 三叶草 → anim_sprout → anim_face`
—— 三叶草长在脑袋**后面**，脸与那支小叶子挡在前面，与豌豆射手自己的叶子同一个层级口径。
三叶草轨道的内容帧区间固定为 `29..103`（与豌豆射手的头部层一致），
所以 body 实例（帧 4..28）与眨眼实例（帧 1..3）都不会画到它。

为什么必须另存一个文件、而不是改 PeaShooterSingle.reanim：
  该文件被 REANIM_PEASHOOTER 共享（普通豌豆射手、卡面、图鉴、光标预览都在用），
  加轨道会让原版豌豆射手也长出三叶草。独立文件 + 独立定义槽（Reanimator.cpp）
  让原版豌豆射手零影响，也让贴图可以**直接写在文件里**（不需要运行期换图）。

落位参数是**烘焙进文件**的（reanim 没有"运行期改轨道变换"的入口），
所以调参就是改本文件顶部的常量后重跑：

    python scripts/gen-wind-peashooter-reanim.py            # 重新生成（会覆盖目标文件）
    python scripts/gen-wind-peashooter-reanim.py --check    # 只校验：已提交的文件与生成结果一致

    （--check 还会做结构自检：三叶草的内容帧区间必须恰好是 29..103、
      必须排在 anim_sprout / anim_face **之前**，源文件结构变了会直接报错。）

注意：PeaShooterSingle.reanim 或 Blover.reanim 若以后被改动，需要重跑本脚本。
"""

import argparse
import math
import os
import sys
import xml.etree.ElementTree as ET

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE_PATH = os.path.join(REPO_ROOT, 'res', 'main', 'reanim', 'PeaShooterSingle.reanim')
BLOVER_PATH = os.path.join(REPO_ROOT, 'res', 'main', 'reanim', 'Blover.reanim')
TARGET_PATH = os.path.join(REPO_ROOT, 'res', 'main', 'reanim', 'WindPeashooter.reanim')

CLOVER_IMAGE = 'IMAGE_REANIM_BLOVER_PETAL'
PETAL_WIDTH = 33.0
PETAL_HEIGHT = 34.0

# ---------------------------------------------------------------------------
# 三叶草落位参数（唯一的调参入口）
# ---------------------------------------------------------------------------
# 三片叶子照抄 Blover.reanim 第 0 帧（= 三叶草风扇并拢、正面朝上的那一帧）。
BLOVER_PETALS = ('Blover_petals', 'Blover_petals2', 'Blover_petals3')
BLOVER_PETAL_FRAME = 0

# 三叶草整体缩放：0.68 → 每片叶子约 22 x 23 px，整株三叶草约 40 x 40 px
# （豌豆射手的脑袋绘制尺寸约 39 x 36 px，所以三叶草与脑袋同级）。
CLOVER_SCALE = 0.68
# 三叶草整体额外旋转（度）。0 = 与 Blover 一样"上、左下、右下"三片。
CLOVER_ROTATION = 0.0
# 三叶草中心相对 **sprout（豌豆射手头顶那支小叶子）中心** 的偏移（reanim 像素，+右/+下）。
# 当前值让三片叶子"贴着脑袋左上后方长出来"：上面的叶片压在脑袋顶的左上角，
# 左边的叶片贴着脑袋左缘（与植株自己那支小叶子重叠），第三片被脑袋挡住。
# ★ 这个 X 是按实机反馈调过的（一开始 1.0 时三片叶子离脑袋太远，看起来像飘在旁边）。
CLOVER_OFFSET_X = 7.0
CLOVER_OFFSET_Y = -8.0

# 三叶草挂在哪条轨道上（位置与倾角逐帧跟随它）。改成 anim_face 就会跟着脑袋走。
ANCHOR_TRACK = 'anim_sprout'
ANCHOR_WIDTH = 18.0          # anim_sprout.png
ANCHOR_HEIGHT = 14.0

# 豌豆射手的头部层：anim_head_idle(29..53) / anim_shooting(54..78) / anim_full_idle(79..103)
HEAD_FIRST = 29
HEAD_LAST = 103
TRACK_FRAME_COUNT = 104

# 三叶草轨道名（1/2/3 对应 Blover 的三片叶子）
CLOVER_TRACKS = ('WindPeashooter_clover1', 'WindPeashooter_clover2', 'WindPeashooter_clover3')

FLOAT_FIELDS = ('x', 'y', 'kx', 'ky', 'sx', 'sy', 'f', 'a')


def read_text(path):
    # newline='' → 原样保留 CRLF（reanim 文件是 CRLF，生成结果也要保持一致）
    with open(path, 'r', encoding='utf-8', newline='') as handle:
        return handle.read()


def load_tracks(text):
    """把 reanim XML 解析成 {轨道名: [每帧的原始字段 dict]}（未做继承）。"""
    root = ET.fromstring('<root>' + text + '</root>')
    tracks = {}
    for track in root.findall('track'):
        name = track.find('name').text
        tracks[name] = [{child.tag: child.text for child in frame} for frame in track.findall('t')]
    return tracks


def resolve_track(frames):
    """按 ReanimationLoadDefinition 的"缺省字段沿用上一帧"规则解析出每帧的有效值。

    起点与原版一致：x/y/kx/ky=0、sx/sy=1、f=0、a=1。
    f<0 表示该帧是空白帧（不画）。
    """
    previous = {'x': 0.0, 'y': 0.0, 'kx': 0.0, 'ky': 0.0, 'sx': 1.0, 'sy': 1.0, 'f': 0.0, 'a': 1.0}
    resolved = []
    for frame in frames:
        current = dict(previous)
        for field in FLOAT_FIELDS:
            if field in frame:
                current[field] = float(frame[field])
        previous = current
        resolved.append(current)
    return resolved


def content_range(resolved):
    """一条轨道"有内容"的帧区间（对应 Reanimation::GetFramesForLayer 的窗口算法）。"""
    visible = [index for index, frame in enumerate(resolved) if frame['f'] >= 0.0]
    if not visible:
        return None
    return visible[0], visible[-1]


def fmt(value):
    """与既有 reanim 文件一样的紧凑写法（50 而不是 50.0）。"""
    text = '%g' % round(value, 6)
    return '0' if text == '-0' else text


def image_center(transform, width, height):
    """引擎口径下这张图**绘制后的中心**（x/y 是"绕左上角旋转"的那个角）。

    Reanimation::DrawTrack 的线性部分是 [[cos(-kx)*sx, sin(-ky)*sy],
    [-sin(-kx)*sx, cos(-ky)*sy]]，所以局部点 (u, v) 落到
      (m00*u + m01*v + x, m10*u + m11*v + y)。
    """
    a_skew_x = math.radians(-transform['kx'])
    a_skew_y = math.radians(-transform['ky'])
    m00 = math.cos(a_skew_x) * transform['sx']
    m01 = math.sin(a_skew_y) * transform['sy']
    m10 = -math.sin(a_skew_x) * transform['sx']
    m11 = math.cos(a_skew_y) * transform['sy']
    u = width * 0.5
    v = height * 0.5
    return (m00 * u + m01 * v + transform['x'], m10 * u + m11 * v + transform['y'])


def place_image(center_x, center_y, kx, ky, sx, sy, width, height):
    """反解出"让这张图的绘制中心落在 (center_x, center_y)"时的 x/y。"""
    a_skew_x = math.radians(-kx)
    a_skew_y = math.radians(-ky)
    m00 = math.cos(a_skew_x) * sx
    m01 = math.sin(a_skew_y) * sy
    m10 = -math.sin(a_skew_x) * sx
    m11 = math.cos(a_skew_y) * sy
    u = width * 0.5
    v = height * 0.5
    return (center_x - (m00 * u + m01 * v), center_y - (m10 * u + m11 * v))


def clover_local_geometry(blover_tracks):
    """Blover 第 0 帧里三片叶子相对"三叶草中心"的布局（中心 / 角度 / 缩放）。

    三叶草中心取三片叶子绘制中心的平均值，于是"整体缩放 + 整体旋转"是绕三叶草自己的中心做的。
    """
    petals = []
    for name in BLOVER_PETALS:
        if name not in blover_tracks:
            raise SystemExit('Blover.reanim 缺少轨道 %s（被改动过？）' % name)
        resolved = resolve_track(blover_tracks[name])
        if content_range(resolved) is None or BLOVER_PETAL_FRAME >= len(resolved):
            raise SystemExit('轨道 %s 在 Blover.reanim 里没有内容。' % name)
        transform = resolved[BLOVER_PETAL_FRAME]
        petals.append((transform, image_center(transform, PETAL_WIDTH, PETAL_HEIGHT)))

    center_x = sum(center[0] for _, center in petals) / len(petals)
    center_y = sum(center[1] for _, center in petals) / len(petals)
    return petals, (center_x, center_y)


def clover_frames(anchor_face, petals, clover_center, first, last):
    """算出三片叶子在 [first, last] 上每一帧的 x/y/kx/ky/sx/sy。

    三叶草的整体变换 = 等比缩放 + 绕三叶草中心旋转 + 平移到 sprout 中心（含偏移）。
    整体旋转角 = CLOVER_ROTATION + sprout 当帧的倾角 —— 于是三叶草跟着那支小叶子一起摆，
    而且**每个动作段内部天然无缝**（sprout 自己的动画在每个段里都是循环的）。
    """
    frames = {}
    for index in range(first, last + 1):
        anchor = anchor_face[index]
        anchor_center = image_center(anchor, ANCHOR_WIDTH, ANCHOR_HEIGHT)
        target_x = anchor_center[0] + CLOVER_OFFSET_X
        target_y = anchor_center[1] + CLOVER_OFFSET_Y

        k_add = CLOVER_ROTATION + anchor['kx']
        angle = math.radians(-k_add)      # 引擎里 kx 的线性部分是绕 -kx 的旋转
        cos_a = math.cos(angle)
        sin_a = math.sin(angle)

        for track_index, (transform, center) in enumerate(petals):
            dx = (center[0] - clover_center[0]) * CLOVER_SCALE
            dy = (center[1] - clover_center[1]) * CLOVER_SCALE
            petal_center = (target_x + cos_a * dx + sin_a * dy,
                            target_y - sin_a * dx + cos_a * dy)

            kx = transform['kx'] + k_add
            ky = transform['ky'] + k_add
            sx = transform['sx'] * CLOVER_SCALE
            sy = transform['sy'] * CLOVER_SCALE
            x, y = place_image(petal_center[0], petal_center[1], kx, ky, sx, sy, PETAL_WIDTH, PETAL_HEIGHT)
            frames.setdefault(track_index, {})[index] = {
                'x': x, 'y': y, 'kx': kx, 'ky': ky, 'sx': sx, 'sy': sy,
            }
    return frames


def build_track(track_name, image_name, frames_by_index, first, last):
    """把一条新轨道渲染成 reanim XML 文本。

    帧标记约定与既有轨道完全一致：**第 0 帧 <f>-1</f>（空白起）**、首个内容帧 <f>0</f>、
    内容区间**之后的第一帧** <f>-1</f>（结束可见段），其余 <t></t> 空白帧。
    （第 0 帧那条 -1 不能省：ReanimationFillInMissingData 的 f 起点是 0，
      没有它时 first 之前的空白帧会被读成"可见"——虽然那时还没有贴图、
      DrawTrack 仍然画不出东西，但轨道自己的可见帧区间就不再等于它声称的区间了。）
    """
    lines = ['<track>', '<name>%s</name>' % track_name]
    for index in range(TRACK_FRAME_COUNT):
        if first <= index <= last:
            values = frames_by_index[index]
            tail = '<f>0</f><i>%s</i>' % image_name if index == first else ''
            lines.append(
                '<t><x>%s</x><y>%s</y><kx>%s</kx><ky>%s</ky><sx>%s</sx><sy>%s</sy>%s</t>'
                % (fmt(values['x']), fmt(values['y']), fmt(values['kx']), fmt(values['ky']),
                   fmt(values['sx']), fmt(values['sy']), tail))
        elif index == 0 or index == last + 1:
            lines.append('<t><f>-1</f></t>')
        else:
            lines.append('<t></t>')
    lines.append('</track>')
    return '\r\n'.join(lines) + '\r\n'


def validate(source_tracks, blover_tracks, source_text):
    """核对源文件结构；任何一条不符就报错，而不是悄悄生成错位的三叶草。"""
    sprout = content_range(resolve_track(source_tracks[ANCHOR_TRACK])) if ANCHOR_TRACK in source_tracks else None
    if sprout is None:
        raise SystemExit('PeaShooterSingle.reanim 里找不到有内容的 %s（被改动过？）' % ANCHOR_TRACK)
    if sprout[0] != HEAD_FIRST or sprout[1] != HEAD_LAST:
        raise SystemExit(
            '%s 的内容区间是 %s，期望 %d..%d —— PeaShooterSingle.reanim 被改动过，'
            '请重新核对本脚本的 HEAD_* 常量。' % (ANCHOR_TRACK, sprout, HEAD_FIRST, HEAD_LAST))

    face = content_range(resolve_track(source_tracks['anim_face']))
    if face != (HEAD_FIRST, HEAD_LAST):
        raise SystemExit('anim_face 的内容区间是 %s，期望 %d..%d。' % (face, HEAD_FIRST, HEAD_LAST))

    # 三叶草必须排在 sprout / anim_face **之前**（绘制顺序 = 文件里的轨道顺序）：
    # 这样它是"长在脑袋后面"的叶子，脸挡住前面。
    sprout_at = source_text.find('<name>%s</name>' % ANCHOR_TRACK)
    face_at = source_text.find('<name>anim_face</name>')
    if sprout_at < 0 or face_at < 0:
        raise SystemExit('源文件里找不到 %s / anim_face 轨道。' % ANCHOR_TRACK)
    if sprout_at > face_at:
        raise SystemExit('源文件里 %s 排在 anim_face 之后，绘制顺序假设不再成立。' % ANCHOR_TRACK)

    for name in CLOVER_TRACKS:
        if name in source_tracks:
            raise SystemExit('%s 已经出现在源文件里了。' % name)

    for name in BLOVER_PETALS:
        resolved = resolve_track(blover_tracks[name])
        if BLOVER_PETAL_FRAME >= len(resolved) or resolved[BLOVER_PETAL_FRAME]['f'] < 0.0:
            raise SystemExit('Blover.reanim 第 %d 帧的 %s 是空白帧。' % (BLOVER_PETAL_FRAME, name))
        referenced = {frame.get('i') for frame in blover_tracks[name] if frame.get('i')}
        if referenced != {CLOVER_IMAGE}:
            raise SystemExit('%s 引用的贴图是 %s，期望只有 %s。'
                             % (name, sorted(referenced), CLOVER_IMAGE))


def generate():
    source = read_text(SOURCE_PATH)
    source_tracks = load_tracks(source)
    blover_tracks = load_tracks(read_text(BLOVER_PATH))

    validate(source_tracks, blover_tracks, source)

    petals, clover_center = clover_local_geometry(blover_tracks)
    anchor_face = resolve_track(source_tracks[ANCHOR_TRACK])
    frames = clover_frames(anchor_face, petals, clover_center, HEAD_FIRST, HEAD_LAST)

    clover_text = ''
    for track_index, track_name in enumerate(CLOVER_TRACKS):
        clover_text += build_track(track_name, CLOVER_IMAGE, frames[track_index], HEAD_FIRST, HEAD_LAST)

    # 插到 anim_sprout 那条轨道**之前**：于是绘制顺序是 stem → 三叶草 → sprout → face
    insert_at = source.find('<track>\r\n<name>%s</name>' % ANCHOR_TRACK)
    if insert_at < 0:
        insert_at = source.find('<track>\n<name>%s</name>' % ANCHOR_TRACK)
    if insert_at < 0:
        raise SystemExit('在源文件里定位 %s 轨道失败。' % ANCHOR_TRACK)

    return source[:insert_at] + clover_text + source[insert_at:]


def main():
    parser = argparse.ArgumentParser(description='生成 / 校验 res/main/reanim/WindPeashooter.reanim')
    parser.add_argument('--check', action='store_true',
                        help='只校验已提交文件与生成结果一致（不做写入）')
    args = parser.parse_args()

    expected = generate()

    # 结构自检：生成结果里三叶草的内容帧区间、顺序与条数都必须符合预期
    generated = load_tracks(expected)
    names = [track.findtext('name') for track in ET.fromstring('<root>' + expected + '</root>').findall('track')]
    for name in CLOVER_TRACKS:
        if content_range(resolve_track(generated[name])) != (HEAD_FIRST, HEAD_LAST):
            raise SystemExit('%s 的内容帧区间不是 %d..%d。' % (name, HEAD_FIRST, HEAD_LAST))
        if names.index(name) > names.index(ANCHOR_TRACK):
            raise SystemExit('%s 必须排在 %s 之前（绘制顺序 = 轨道顺序）。' % (name, ANCHOR_TRACK))

    if args.check:
        if not os.path.exists(TARGET_PATH):
            raise SystemExit('%s 不存在，请先运行本脚本（不带 --check）。' % TARGET_PATH)
        actual = read_text(TARGET_PATH)
        if actual != expected:
            actual_lines = actual.split('\r\n')
            expected_lines = expected.split('\r\n')
            for index in range(max(len(actual_lines), len(expected_lines))):
                a = actual_lines[index] if index < len(actual_lines) else '<缺行>'
                b = expected_lines[index] if index < len(expected_lines) else '<缺行>'
                if a != b:
                    raise SystemExit(
                        'WindPeashooter.reanim 与生成结果不一致（首个差异在第 %d 行）：\n  文件: %s\n  期望: %s'
                        % (index + 1, a, b))
            raise SystemExit('WindPeashooter.reanim 与生成结果不一致（仅行尾差异）。')
        print('WindPeashooter.reanim is up to date (%d tracks, %d lines).'
              % (len(names), len(actual.split('\r\n'))))
        return

    with open(TARGET_PATH, 'w', encoding='utf-8', newline='') as handle:
        handle.write(expected)
    print('Wrote %s (%d tracks, %d lines).'
          % (os.path.relpath(TARGET_PATH, REPO_ROOT), len(names), len(expected.split('\r\n'))))


if __name__ == '__main__':
    sys.exit(main())
