# 固定四阶辣椒投手移植记录

辣椒投手使用用户提供的国服《植物大战僵尸 2》4.2.4 素材，固定为四阶，只在斗蛐蛐 2 第二选卡页出现，排在回旋镖射手之后。不提供阶数切换、种植觉醒、能量豆或能量豆地块效果。

## 战斗口径

| 项目 | 本项目实现 |
| --- | --- |
| 阳光／冷却／生命 | 200／5 秒／750 |
| 普攻 | 本行有目标才起手，约每 290 tick 投出一枚蓝焰辣椒；PAM 攻击段第 268–327 帧，第 281 帧动作点出弹 |
| 命中 | 直击 125 点；落点周围 3×3 格内其他僵尸各受 25 点，直击目标只结算一次 |
| 蓝焰 | 受到直击或溅射且仍存活的僵尸每 10 tick 受到 10 点本体伤害，持续 400 tick；再次命中刷新时长而不叠加伤害 |

基本数值、四阶 250% 攻防和继承的蓝焰每秒 100 点取自[指定 Wiki](https://plantsvszombies.wiki.gg/wiki/Pepper-pult)。蓝焰持续 4 秒是本项目约定，Wiki 未给出时长。弹丸沿用项目的投手弹道与目标伤害资格、护甲和免疫规则；蓝焰会移除僵尸已有的减速／冻结效果。项目当前没有植物受冻状态，因此保温与植物解冻没有可作用的对象。

## 素材与存档

`scripts/gen-pepper-pult-reanim.py` 校验四份 PAM JSON 的 SHA-256、标签、普通攻击第 281 帧动作点及被引用 PNG 的合并 SHA-256，生成本体、蓝焰弹丸、蓝色普通命中和蓝焰灼烧四份 `.reanim` 与相应 PNG。主体只转换待机、普通攻击和水面片段，不转换能量豆片段。

```bash
python3 scripts/gen-pepper-pult-reanim.py --source /Users/fiture/Privates/pvz-2-crack/work/pepperpult --check
python3 tools/reanim-preview.py res/main/reanim/PepperPult.reanim /tmp/pepper-idle.png 18 2
python3 tools/reanim-preview.py res/main/reanim/PepperPult.reanim /tmp/pepper-attack.png 110 2
python3 tools/reanim-preview.py res/main/reanim/PepperPultProjectile.reanim /tmp/pepper-projectile.png 3 3
python3 tools/reanim-preview.py res/main/reanim/PepperPultHit.reanim /tmp/pepper-hit.png 10 2
python3 tools/reanim-preview.py res/main/reanim/PepperPultBlueBurn.reanim /tmp/pepper-burn.png 12 2
cmake --build build
```

灼烧剩余时间、距下次伤害的 tick 数及跟随僵尸的蓝焰动画附件 ID 存于僵尸 `.v4` 可选 TLV 字段 `7`。旧存档缺少此字段时按未灼烧初始化。弹丸沿用已有投手弹丸状态，无新增弹丸存档字段。

## 实机验收

编译和离线预览无法替代游戏内检查。需在斗蛐蛐 2 验证第二选卡页顺序、费用与冷却、卡面及战场比例、每轮单发、泳池／屋顶弹道、直击与溅射各一次结算、蓝焰续时及死亡清除。还需分别在弹丸飞行和灼烧中保存／读取 `.v4`，并核对缺少字段 `7` 的旧存档及普通模式不可选。
