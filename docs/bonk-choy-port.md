# 五阶菜问移植记录

菜问固定为国服《植物大战僵尸 2》五阶，仅在斗蛐蛐 2 第二选卡页、橡木弓手之后出现。不接入其它阶数形态、种植觉醒、能量豆或能量豆地块效果。
普通拳、五阶强化和撼地拳参照[指定 Wiki](https://plantsvszombies.wiki.gg/wiki/Bonk_Choy_%28PvZ2%29)；上勾拳 120 点及撼地拳不眩晕按本次约定。

## 战斗口径

| 项目 | 实现 |
| --- | --- |
| 阳光 / 卡片冷却 / 生命 | 150 / 5 秒 / 900 |
| 普通拳 | 本行前后各一格内取最近目标；约每 0.4 秒造成 45 点伤害 |
| 上勾拳 | 每第 7 拳改为 120 点伤害；可浮空目标离地并暂停行动 60 tick |
| 撼地拳 | 两次上勾拳后立即追加；右侧一格主目标 145 点、同格其它目标各 45 点，无眩晕 |

上勾拳、撼地拳的次数存入植物 `.v4` 可选 TLV 字段 `103`；僵尸浮空剩余时间存入僵尸字段 `5`。旧存档缺少字段时从零开始。近战伤害沿用现有僵尸护甲规则。

## 资源与动画

`scripts/gen-bonk-choy-reanim.py` 从用户提供的 `bonkchoy` 目录读取 `BONKCHOY` PAM 与裁切 PNG，锁定源哈希、30 FPS、标签和动作点，只输出实际引用的本体部件。运行时不读取提取目录。逐帧预览表明：`attack` / `attack2` 是向右 / 向左普通拳，`attack4` / `attack5` 是向右 / 向左上勾拳，`attack6` 是向右撼地拳；`attack7` 则是本次不使用的向左撼地拳。PAM 的 RGB 闪光渐变无法由 reanim 的逐层颜色表达，转换时保留其透明度。

```bash
python3 scripts/gen-bonk-choy-reanim.py --source /path/to/bonkchoy --check
python3 tools/reanim-preview.py res/main/reanim/BonkChoy.reanim /tmp/bonk-idle.png 15 2
python3 tools/reanim-preview.py res/main/reanim/BonkChoy.reanim /tmp/bonk-punch.png 95 2
python3 tools/reanim-preview.py res/main/reanim/BonkChoy.reanim /tmp/bonk-uppercut.png 115 2
python3 tools/reanim-preview.py res/main/reanim/BonkChoy.reanim /tmp/bonk-quake.png 218 2
cmake --build build
```

实机仍需核对前后命中距离、上勾拳和撼地拳落点、浮空与其它控制效果的组合、卡面及战场比例，以及技能动作中保存／读取。离线预览和编译不能替代游戏内验收。
