# 固定四阶潜伏芹菜移植记录

潜伏芹菜使用用户提供的国服《植物大战僵尸 2》素材，固定为四阶，仅在斗蛐蛐 2 第二选卡页出现，排在辣椒投手之后。四阶继承三阶的出土掌击；不提供其它阶数切换、种植觉醒、能量豆及能量豆地块效果。

## 战斗口径

| 项目 | 实现 |
| --- | --- |
| 阳光／冷却／生命 | 50／15 秒／3000 |
| 潜伏 | 种植后下潜；本行僵尸经过植株中心且仍在左侧一格时出土。潜伏及出土途中不成为普通僵尸攻击目标 |
| 连击 | 出土后只攻击本行左侧一格的最近有效目标；每 50 tick 造成 250 点普通近战伤害，每次命中重新索敌 |
| 掌击 | 每次出土独立判定一次 50% 概率；在 `attack_special` 第 558 帧动作点最多击退有效目标 80 像素，位移止于背后攻击边界，沿用 `Zombie::KnockBack` 的免疫规则 |
| 再潜伏 | 约 300 tick 内没有目标时播放下潜动画 |

基础数值、国服四阶 250% 攻防、继承的 50% 一格掌击及左侧一格射程参考[指定 Wiki](https://plantsvszombies.wiki.gg/wiki/Celery_Stalker)。本项目以 100 Hz 游戏逻辑运行；普通连击 PAM 的相邻命中动作点相隔 3 帧，攻击循环按 6 FPS 播放，使每次伤害间隔约 50 tick。

## 资源与存档

`scripts/gen-celery-stalker-reanim.py` 校验用户提供的两份 PAM JSON 哈希、标签与动作点，生成本体、掌击特效及实际引用的 PNG。转换的本体片段只有待机、下潜、地下待机、出土、普通连击及掌击；不转换能量豆与水面片段。源特效的逐帧 RGB 变色无法直接写入 reanim，转换保留透明度并在运行时加固定黄色调。

```bash
python3 scripts/gen-celery-stalker-reanim.py --source /Users/fiture/Privates/pvz-2-crack/work/celerystalker --check
python3 tools/reanim-preview.py res/main/reanim/CeleryStalker.reanim /tmp/celery-idle.png 0 3
python3 tools/reanim-preview.py res/main/reanim/CeleryStalker.reanim /tmp/celery-hidden.png 84 3
python3 tools/reanim-preview.py res/main/reanim/CeleryStalker.reanim /tmp/celery-palm.png 220 3
cmake --build build
```

潜伏、出土、掌击和连击进度只使用已有植物状态、`mStateCountdown` 及 `mShootingCounter`；无需新增 `.v4` 字段。原有枚举整数不变，新值只追加在哨兵之前。

## 实机验收

在斗蛐蛐 2 核对第二页卡位、卡面和战场比例、单目标与多目标索敌、伤害节奏、掌击概率及击退免疫、隐藏与暴露期间的僵尸攻击、目标离开后的再潜伏。分别在下潜、出土和连击期间保存／读取 `.v4`，并确认旧存档及其它模式的选卡行为。
