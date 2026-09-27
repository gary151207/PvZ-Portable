# 一阶椰子加农炮移植记录

**范围：** 国服《植物大战僵尸 2》椰子加农炮的一阶普通攻击。装扮、高阶、能量豆和能量豆地块效果均不接入。  
**入口：** 斗蛐蛐 2 第二选卡页，排在毒液豌豆射手之后。

## 行为

| 项目 | 实现 |
| --- | --- |
| 阳光、卡片冷却、生命 | 400、500 tick、300 |
| 操作 | 点击已装填的植物，沿所在行向右发射一枚炮弹；无目标时也可发射 |
| 命中 | 炮弹碰到本行首个目标后爆炸，该目标受 900 点伤害 |
| 溅射 | 爆炸中心水平约 3 格、上下各 1 行内的其他僵尸各受 300 点伤害 |
| 装填 | 从炮弹发出起计 1500 tick，期间点击不会重复开火 |
| 存档 | 复用植物现有的 `mState`、`mStateCountdown`、`mShootingCounter`，无需新增 TLV 字段 |

数值和一阶行为参考[植物大战僵尸百科的椰子加农炮条目](https://plantsvszombies.wiki.gg/zh/wiki/PvZ2%3A%E6%A4%B0%E5%AD%90%E5%A4%A7%E7%82%AE?variant=zh-hans)。伤害结算沿用本项目的僵尸护甲和爆炸伤害规则。

## 资源

输入是仓库外的 `coconutcannon` PAM 导出目录。`scripts/gen-coconut-cannon-reanim.py` 锁定两份普通攻击 PAM JSON 的 SHA-256、帧率、标签与动作点，仅生成本体和普通炮弹的 `.reanim` 及其引用的 PNG。植物攻击段第 133 帧（段内第 12 帧）执行 `use_action`。弹体和爆炸的 PAM 原点不同，因此转换时分别校准。

```bash
python3 scripts/gen-coconut-cannon-reanim.py --source /path/to/coconutcannon
python3 scripts/gen-coconut-cannon-reanim.py --source /path/to/coconutcannon --check
cmake --build build
```

移植后应在斗蛐蛐 2 实机检查卡面、本体位置、点击区域、炮弹与爆炸原点、3×3 溅射、15 秒装填以及装填中保存和读档。离线动画预览与编译不能代替这些游戏内检查。
