# 固定五阶回旋镖射手移植记录

回旋镖射手使用用户提供的国服《植物大战僵尸 2》4.2.4 素材，固定为五阶，只在斗蛐蛐 2 第二选卡页出现，排在菜问之后。不提供低阶切换、二／三阶的费用联动、种植觉醒、能量豆或能量豆地块效果。

## 战斗口径

| 项目 | 本项目实现 |
| --- | --- |
| 阳光／冷却／生命 | 175／5 秒／900 |
| 普攻 | 本行有目标才起手，约每 290 tick 投出一枚；PAM 攻击段第 52–91 帧，第 72 帧动作点出弹 |
| 回旋镖 | 以 3.33 px/tick 向右飞，去程命中三个目标或到右边界后折返；去程和回程各最多命中三个目标，同一目标每程至多一次，每次 60 伤害，绕过盾牌 |
| 龙卷 | 每轮独立判定 10% 概率；龙卷镖保留普通镖的往返和伤害，命中可受控地面僵尸后施加旋转 |
| 旋转 | 本项目约定为 100 tick，暂停原行动并以 1 px/tick 向右移动；首次撞到同行另一只僵尸时，对被撞者造成 60 伤害及 40 px 击退，随即结束旋转 |

五阶 300% 基础攻防及 10% 龙卷概率取自[指定 Wiki 的国服升级表](https://plantsvszombies.wiki.gg/wiki/Bloomerang_%28PvZ2%29)。旋转时长、位移、撞击伤害和击退距离未在该资料中给出，是本项目约定，不代表已经核实的原版数值。空中单位、载具和 Boss 只受回旋镖伤害，不进入旋转。普通镖和龙卷镖均遵守现有目标、护甲与伤害免疫规则。

## 素材与存档

`scripts/gen-bloomerang-reanim.py` 校验四份 PAM JSON 的 SHA-256、版本、FPS、帧数、主体动作点及全部引用 PNG 的合并 SHA-256，从提供的分片生成本体、飞行镖、龙卷和命中特效四份 `.reanim` 及被引用的 PNG。主体只转换待机、普通攻击和水面片段；不转换装扮或能量豆片段。龙卷源动画中预制的僵尸身体部件被过滤，实际受击僵尸由游戏自身绘制并旋转。

```bash
python3 scripts/gen-bloomerang-reanim.py --source /Users/fiture/Privates/pvz-2-crack/work/bloomerang --check
python3 tools/reanim-preview.py res/main/reanim/Bloomerang.reanim /tmp/bloomerang-idle.png 18 2
python3 tools/reanim-preview.py res/main/reanim/Bloomerang.reanim /tmp/bloomerang-attack.png 72 2
python3 tools/reanim-preview.py res/main/reanim/BloomerangProjectile.reanim /tmp/bloomerang-projectile.png 3 3
python3 tools/reanim-preview.py res/main/reanim/BloomerangTornado.reanim /tmp/bloomerang-tornado.png 6 2
python3 tools/reanim-preview.py res/main/reanim/BloomerangHit.reanim /tmp/bloomerang-hit.png 4 2
cmake --build build
```

弹丸当前行程、发射位置、当前行程的三个命中 ID 存入 `.v4` 弹丸可选 TLV 字段 `101`。旋转剩余时间存入僵尸可选字段 `6`。旧存档缺少这些字段时按未命中、未旋转初始化。发射者死亡或被铲除后，已发出的镖仍继续折返并从左边界离场。

## 实机验收

编译和离线预览无法替代游戏内检查。需在斗蛐蛐 2 验证选卡位置、卡面及战场比例、单目标往返两次命中、三目标上限、重叠目标、盾牌、龙卷概率、旋转撞击一次结算、植物被铲除后的弹道，以及泳池／屋顶的行高。还需在弹丸飞行和僵尸旋转时分别保存／读取，并核对旧 `.v4` 存档。
