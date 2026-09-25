#!/usr/bin/env python3
"""生成 res/main/reanim/ThreeGaling.reanim（三线机枪射手的 reanim）。

三线机枪射手（SEED_THREE_GATLING_PEA）= 三线射手 × 机枪射手 的合成态。它的外观 =
三线射手的三个头 + 每个头一顶机枪头盔 + 每个头一根四段式机枪枪管 + 每个头一层高斯机枪嘴，所以：

  * 骨架直接**复制** reanim/ThreePeater.reanim（149 帧 × 36 条轨道），
    这样头部摆动、倾斜、叶片摇摆、眨眼、开火动作全部免费继承；
  * **原版嘴保持不动**：ThreePeater_mouth（19x43 的"嘴洞"）在机枪射手那边对应的是
    GatlingPea_mouth，是底层；
  * 追加三条**嘴覆层**轨道 ThreeGaling_mouth_overlay1/2/3，引用
    IMAGE_REANIM_THREEGALING_MOUTH_OVERLAY（玩家给的 19x43 绿色唇形，与 ThreePeater_mouth
    **同尺寸**，所以直接沿用嘴轨道的逐帧变换），对应机枪射手的 GatlingPea_mouth_overlay：它画在
    枪管**之后**，于是唇形压在枪管根部（枪从嘴里伸出来）；
  * 追加十二条枪管轨道 ThreeGaling_head{1,2,3}_barrel{3,4,2,1}，引用机枪射手自己的
    IMAGE_REANIM_GATLINGPEA_BARREL（四段式枪管，逐段照抄 GatlingPea.reanim 的几何，
    再按"头部相对偏移 + 头部倾斜 + 头部缩放比"搬到三个小头上）；
  * 追加三条头盔轨道 ThreeGaling_helmet1/2/3，引用 IMAGE_REANIM_THREEGALING_HELMET。

绘制顺序（= 本文件里的轨道顺序）**完全照抄机枪射手**：
    脸 → 嘴 → 枪管 → 嘴覆层 → 头盔。

为什么必须另存一个文件、而不是改 ThreePeater.reanim：
  该文件被 REANIM_THREEPEATER 共享（普通三线射手、图鉴卡面、过场动画都在用），
  加轨道会让原版三线射手也长出头盔、枪管。独立文件 + 独立定义槽（Reanimator.cpp）
  让原版三线射手零影响，也让贴图可以**直接写在文件里**（不需要运行期换图）。

轨道顺序即绘制顺序：本文件是 [三线射手原有 36 条] → [12 条枪管] → [3 条头盔]，
与机枪射手一致（嘴 → 枪管 → 头盔）。

头盔 / 枪管的位置与缩放是**烘焙进文件**的（reanim 没有"运行期改轨道变换"的入口），
所以调参就是改本文件顶部的常量后重跑：

    python scripts/gen-three-galing-reanim.py            # 重新生成（会覆盖目标文件）
    python scripts/gen-three-galing-reanim.py --check    # 只校验：已提交的文件与生成结果一致

    （--check 还会做结构自检：每条头盔/枪管轨道的内容帧区间必须**恰好等于**对应脸轨道的区间，
      且不能与眨眼窗口重叠 —— 前者保证它们跟着头的每种动作出现，后者保证眨眼用的临时动画实例
      不会在旁边多画一顶头盔/一根枪管。）

注意：ThreePeater.reanim 或 GatlingPea.reanim 若以后被改动，需要重跑本脚本。
"""

import argparse
import math
import os
import sys
import xml.etree.ElementTree as ET

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE_PATH = os.path.join(REPO_ROOT, 'res', 'main', 'reanim', 'ThreePeater.reanim')
GATLING_PATH = os.path.join(REPO_ROOT, 'res', 'main', 'reanim', 'GatlingPea.reanim')
TARGET_PATH = os.path.join(REPO_ROOT, 'res', 'main', 'reanim', 'ThreeGaling.reanim')

MOUTH_IMAGE = 'IMAGE_REANIM_THREEPEATER_MOUTH'                    # 原版嘴（= 机枪射手的 mouth），保持不动
MOUTH_OVERLAY_IMAGE = 'IMAGE_REANIM_THREEGALING_MOUTH_OVERLAY'   # 玩家给的唇形覆层（= 机枪射手的 mouth_overlay）
HELMET_IMAGE = 'IMAGE_REANIM_THREEGALING_HELMET'
BARREL_IMAGE = 'IMAGE_REANIM_GATLINGPEA_BARREL'

# ---------------------------------------------------------------------------
# 头盔落位参数（唯一的调参入口）
# ---------------------------------------------------------------------------
# 参考值全部实测自原版机枪射手（GatlingPea.reanim 第 44 帧，正是它"戴好帽子"的那一帧）：
#   脸   = 70x65 的 IMAGE_REANIM_GATLINGPEA_HEAD @ 缩放 (0.555, 0.555) → 绘制 38.85 x 36.08
#   头盔 = 82x87 的 IMAGE_REANIM_GATLINGPEA_HELMET @ 缩放 (0.611, 0.583) → 绘制 50.10 x 50.72
#   → 头盔绘制宽 / 头绘制宽 = 1.288；头盔中心 − 头中心 = (-3.7, -4.7) 像素
# 本植物照这套比例把 1000x1000 的 ThreeGaling_helmet.png 缩到三线射手那个小头上。
GATLING_FACE_W = 38.85
GATLING_FACE_H = 36.08
GATLING_HELMET_W = 50.10
GATLING_HELMET_H = 50.72

HELMET_CANVAS = 1000.0      # ThreeGaling_helmet.png 是 1000x1000 的方图（帽子画在画布里，四周留白）
CAP_WIDTH_RATIO = 1.288     # 帽子绘制宽 / 头部绘制宽
CAP_OFFSET_X = -3.7         # 帽子中心相对头部中心的横移（像素，按头部大小等比缩放）
CAP_OFFSET_Y = -4.7         # 同上，纵向

# 三线射手的头图是 30x30（IMAGE_REANIM_THREEPEATER_HEAD）
HEAD_IMAGE_SIZE = 30.0

# 机枪射手的四段式枪管：**绘制顺序就是源文件里的顺序（3、4、2、1）**，逐段照抄。
# 枪管在源文件里与头刚性绑定（同一帧内 x/y 只随头动、并带一点开火抖动），
# 所以这里用"相对头描点的偏移 + 头部倾斜 + 头部缩放比"把它搬到三线射手的三个头上。
BARREL_SEGMENTS = ('3', '4', '2', '1')
# 枪管微调（默认 0/1.0 = 完全照抄机枪射手的比例；进游戏觉得枪太长/太靠前就改这三个）
BARREL_SCALE = 1.0          # 枪管整体缩放倍数
BARREL_OFFSET_X = 0.0       # 枪管整体横移（植物空间像素）
BARREL_OFFSET_Y = 0.0       # 枪管整体纵移（植物空间像素）
# 机枪射手头的两段窗口（idle 25 帧 + shooting 39 帧，内容区间 29..92），用来做相位映射：
# 本植物的头是 idle 25 帧 + shooting 13 帧，shooting 段按比例拉伸到机枪射手的 39 帧。
GATLING_HEAD_IDLE = (29, 53)
GATLING_HEAD_SHOOTING = (54, 92)

# 每个头要用到的源轨道与要新增的轨道。
#   头 1：idle 窗口 4..28  + shooting 窗口 29..41
#   头 3：idle 窗口 45..69 + shooting 窗口 70..82
#   头 2：idle 窗口 86..110 + shooting 窗口 111..123
#   （眨眼窗口分别是 1..3 / 42..44 / 83..85，都在各自内容区间之外）
# 括号里是**期望的**内容帧区间（会与源文件实际值核对）。
HEADS = (
    dict(head='1', face='anim_face1', idle='anim_head_idle1', shooting='anim_shooting1', blink='anim_blink1',
         mouth='ThreePeater_mouth1', overlay='ThreeGaling_mouth_overlay1', helmet='ThreeGaling_helmet1', first=4, last=41),
    dict(head='3', face='anim_face3', idle='anim_head_idle3', shooting='anim_shooting3', blink='anim_blink3',
         mouth='ThreePeater_mouth3', overlay='ThreeGaling_mouth_overlay3', helmet='ThreeGaling_helmet3', first=45, last=82),
    dict(head='2', face='anim_face2', idle='anim_head_idle2', shooting='anim_shooting2', blink='anim_blink2',
         mouth='ThreePeater_mouth2', overlay='ThreeGaling_mouth_overlay2', helmet='ThreeGaling_helmet2', first=86, last=123),
)

TRACK_FRAME_COUNT = 149

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
    返回 [{字段: 值}]，其中 f<0 表示该帧是空白帧（不画）。
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


def helmet_frames(face_resolved, first, last):
    """算出头盔轨道在 [first, last] 上每一帧的 x/y/kx/ky/sx/sy。

    头盔中心 = 头部绘制矩形的中心 + (CAP_OFFSET_X, CAP_OFFSET_Y) * r（r = 头部大小之比），
    头盔绘制尺寸 = 机枪射手头盔的绘制尺寸 * r；kx/ky 直接抄脸轨道，让帽子跟头一起倾斜。
    """
    result = {}
    for index in range(first, last + 1):
        face = face_resolved[index]
        head_w = HEAD_IMAGE_SIZE * face['sx']
        head_h = HEAD_IMAGE_SIZE * face['sy']
        ratio = head_w / GATLING_FACE_W

        helmet_w = GATLING_HELMET_W * ratio
        helmet_h = GATLING_HELMET_H * ratio

        center_x = face['x'] + head_w * 0.5 + CAP_OFFSET_X * ratio
        center_y = face['y'] + head_h * 0.5 + CAP_OFFSET_Y * ratio

        result[index] = {
            'x': center_x - helmet_w * 0.5,
            'y': center_y - helmet_h * 0.5,
            'kx': face['kx'],
            'ky': face['ky'],
            'sx': helmet_w / HELMET_CANVAS,
            'sy': helmet_h / HELMET_CANVAS,
        }
    return result


def gatling_phase_frame(index, idle_first, idle_last, shooting_first, last):
    """把本植物的头部帧号映射到机枪射手的头部帧号（idle 1:1、shooting 按比例拉伸）。"""
    if index <= idle_last:
        return GATLING_HEAD_IDLE[0] + (index - idle_first)
    our_span = last - shooting_first
    their_span = GATLING_HEAD_SHOOTING[1] - GATLING_HEAD_SHOOTING[0]
    offset = round((index - shooting_first) * their_span / our_span) if our_span else 0
    return GATLING_HEAD_SHOOTING[0] + offset


def barrel_frames(face_resolved, gatling_face, gatling_barrel, idle_first, idle_last, shooting_first, last):
    """算出某一段枪管在本植物某个头上每一帧的变换。

    机枪射手的头没有旋转（kx/ky = 0），所以"枪管相对头描点的偏移"可以直接当作
    头局部坐标里的向量：先按头部大小比 r 缩放，再按本植物头的倾斜角旋转，最后加到头描点上。
    枪管自己的 kx/ky 叠加上头的倾斜，于是整把枪跟着头一起瞄。
    """
    result = {}
    for index in range(idle_first, last + 1):
        face = face_resolved[index]
        g_index = gatling_phase_frame(index, idle_first, idle_last, shooting_first, last)
        g_face = gatling_face[g_index]
        g_barrel = gatling_barrel[g_index]

        ratio = (HEAD_IMAGE_SIZE * face['sx']) / GATLING_FACE_W * BARREL_SCALE
        # MatrixFromTransform: 线性部分 = Rot(kx) · diag(sx, sy)，所以用 kx 当旋转角
        theta = math.radians(face['kx'])
        dx = (g_barrel['x'] - g_face['x']) * ratio
        dy = (g_barrel['y'] - g_face['y']) * ratio

        result[index] = {
            'x': face['x'] + dx * math.cos(theta) - dy * math.sin(theta) + BARREL_OFFSET_X,
            'y': face['y'] + dx * math.sin(theta) + dy * math.cos(theta) + BARREL_OFFSET_Y,
            'kx': face['kx'] + g_barrel['kx'],
            'ky': face['ky'] + g_barrel['ky'],
            'sx': g_barrel['sx'] * ratio,
            'sy': g_barrel['sy'] * ratio,
        }
    return result


def build_track(track_name, image_name, frames_by_index, first, last):
    """把一条新轨道渲染成 reanim XML 文本。

    帧标记约定与既有轨道完全一致：第 0 帧 <f>-1</f>（空白起）、首个内容帧 <f>0</f>、
    内容区间**之后的第一帧** <f>-1</f>（结束可见段），其余 <t></t> 空白帧。
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
        elif index == last + 1:
            lines.append('<t><f>-1</f></t>')
        else:
            lines.append('<t></t>')
    lines.append('</track>')
    return '\r\n'.join(lines) + '\r\n'


def validate(source_tracks, gatling_tracks):
    """核对源文件结构；任何一条不符就报错，而不是悄悄生成错位的头盔/枪管/唇形。"""
    for head in HEADS:
        head_no, expect_first, expect_last = head['head'], head['first'], head['last']
        for name in (head['face'], head['idle'], head['shooting'], head['blink'], head['mouth']):
            if name not in source_tracks:
                raise SystemExit('ThreePeater.reanim 缺少轨道 %s（被改动过？）' % name)

        idle = content_range(resolve_track(source_tracks[head['idle']]))
        shooting = content_range(resolve_track(source_tracks[head['shooting']]))
        face = content_range(resolve_track(source_tracks[head['face']]))
        if idle is None or shooting is None:
            raise SystemExit('轨道 %s / %s 在 ThreePeater.reanim 里是空的。' % (head['idle'], head['shooting']))
        if idle[0] != expect_first or shooting[1] != expect_last or face != (expect_first, expect_last):
            raise SystemExit(
                '头 %s 的窗口与预期不符（face=%s idle=%s shooting=%s，期望 %s..%s）—— '
                'ThreePeater.reanim 被改动过，请重新核对本脚本的 HEADS 表。'
                % (head_no, face, idle, shooting, expect_first, expect_last))
        # 唇形覆层直接沿用嘴轨道的逐帧变换（两张图同为 19x43），所以嘴轨道的区间也必须一致：
        # 这样覆层才会出现在与嘴完全相同的位置与帧区间上。
        if content_range(resolve_track(source_tracks[head['mouth']])) != face:
            raise SystemExit('轨道 %s 的内容区间与 %s 不同，唇形覆层无法逐帧沿用。' % (head['mouth'], head['face']))

        # 眨眼用的是一条独立动画实例，窗口是 anim_blinkN；头盔/枪管/唇形不能在那里出现，
        # 否则眨眼时会重复画一顶帽子 / 一根枪管 / 一层唇。
        blink = content_range(resolve_track(source_tracks[head['blink']]))
        if blink is None:
            raise SystemExit('轨道 %s 在 ThreePeater.reanim 里是空的。' % head['blink'])
        if not (blink[1] < expect_first or blink[0] > expect_last):
            raise SystemExit(
                '眨眼窗口 %s 与 %s 的内容区间 %s..%s 重叠，头盔/枪管/唇形会在眨眼时被重复绘制。'
                % (blink, head['face'], expect_first, expect_last))

    missing = [name for name in ('anim_face', 'anim_head_idle', 'anim_shooting') if name not in gatling_tracks]
    for segment in BARREL_SEGMENTS:
        name = 'GatlingPea_barrel' + segment
        if name not in gatling_tracks:
            missing.append(name)
    if missing:
        raise SystemExit('GatlingPea.reanim 缺少轨道：%s' % ', '.join(missing))

    g_idle = content_range(resolve_track(gatling_tracks['anim_head_idle']))
    g_shooting = content_range(resolve_track(gatling_tracks['anim_shooting']))
    g_face = content_range(resolve_track(gatling_tracks['anim_face']))
    if g_idle != GATLING_HEAD_IDLE or g_shooting != GATLING_HEAD_SHOOTING or g_face != (GATLING_HEAD_IDLE[0], GATLING_HEAD_SHOOTING[1]):
        raise SystemExit(
            'GatlingPea.reanim 的头部窗口变了（idle=%s shooting=%s face=%s，期望 %s / %s）——'
            ' 请更新本脚本的 GATLING_HEAD_* 常量与枪管几何。' % (g_idle, g_shooting, g_face, GATLING_HEAD_IDLE, GATLING_HEAD_SHOOTING))

    for segment in BARREL_SEGMENTS:
        name = 'GatlingPea_barrel' + segment
        if content_range(resolve_track(gatling_tracks[name])) != g_face:
            raise SystemExit('轨道 %s 的内容区间与 anim_face 不同，枪管几何假设不再成立。' % name)


def generate():
    source = read_text(SOURCE_PATH)
    source_tracks = load_tracks(source)
    gatling_tracks = load_tracks(read_text(GATLING_PATH))

    validate(source_tracks, gatling_tracks)

    # 原版嘴**保持不动**（它在机枪射手那边对应 GatlingPea_mouth，是底层）；
    # 玩家给的唇形作为 mouth_overlay 另加一层，绘制在枪管之后。
    if source.count(MOUTH_IMAGE) != 3:
        raise SystemExit('源文件里 %s 的出现次数不是 3（三条嘴轨道各一次）。' % MOUTH_IMAGE)

    output = source
    if not output.endswith('\r\n'):
        output += '\r\n'

    gatling_face = resolve_track(gatling_tracks['anim_face'])
    gatling_barrels = {
        segment: resolve_track(gatling_tracks['GatlingPea_barrel' + segment])
        for segment in BARREL_SEGMENTS
    }

    for head in HEADS:
        head_no, first, last = head['head'], head['first'], head['last']
        face_resolved = resolve_track(source_tracks[head['face']])
        idle_last = first + 24          # 头部的 idle 窗口固定 25 帧（与源文件核对过）
        shooting_first = idle_last + 1

        # 顺序即绘制顺序，照抄机枪射手：枪管（4 段）→ 嘴覆层 → 头盔。
        for segment in BARREL_SEGMENTS:
            track_name = 'ThreeGaling_head%s_barrel%s' % (head_no, segment)
            if track_name in source_tracks or track_name in output:
                raise SystemExit('%s 已经出现在文件里了。' % track_name)
            frames = barrel_frames(face_resolved, gatling_face, gatling_barrels[segment],
                                   first, idle_last, shooting_first, last)
            output += build_track(track_name, BARREL_IMAGE, frames, first, last)

        # 嘴覆层：与机枪射手的 GatlingPea_mouth_overlay 同一层级（画在枪管之后、头盔之前），
        # 变换逐帧沿用原版嘴轨道 —— 两张图同为 19x43，所以唇形正好落在嘴洞上。
        overlay_name = head['overlay']
        if overlay_name in source_tracks or overlay_name in output:
            raise SystemExit('%s 已经出现在文件里了。' % overlay_name)
        output += build_track(overlay_name, MOUTH_OVERLAY_IMAGE, resolve_track(source_tracks[head['mouth']]), first, last)

        helmet_name = head['helmet']
        if helmet_name in source_tracks or helmet_name in output:
            raise SystemExit('%s 已经出现在文件里了。' % helmet_name)
        frames = helmet_frames(face_resolved, first, last)
        output += build_track(helmet_name, HELMET_IMAGE, frames, first, last)

    return output


def main():
    parser = argparse.ArgumentParser(description='生成 / 校验 res/main/reanim/ThreeGaling.reanim')
    parser.add_argument('--check', action='store_true',
                        help='只校验已提交文件与生成结果一致（不做写入）')
    args = parser.parse_args()

    expected = generate()

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
                        'ThreeGaling.reanim 与生成结果不一致（首个差异在第 %d 行）：\n  文件: %s\n  期望: %s'
                        % (index + 1, a, b))
            raise SystemExit('ThreeGaling.reanim 与生成结果不一致（仅行尾差异）。')
        print('ThreeGaling.reanim is up to date (%d lines).' % len(actual.split('\r\n')))
        return

    with open(TARGET_PATH, 'w', encoding='utf-8', newline='') as handle:
        handle.write(expected)
    print('Wrote %s (%d lines).' % (os.path.relpath(TARGET_PATH, REPO_ROOT), len(expected.split('\r\n'))))


if __name__ == '__main__':
    sys.exit(main())
