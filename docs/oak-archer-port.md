# 五阶橡木弓手移植记录

橡木弓手固定为《植物大战僵尸 2》国服五阶，只在斗蛐蛐 2 第二选卡页出现，排在胡萝卜导弹车之后。继承二阶分裂和三阶穿透，但不提供其它阶数形态、种植觉醒、能量豆及能量豆地块效果。

## 战斗口径

| 项目 | 实现 |
| --- | --- |
| 阳光 / 卡片冷却 / 生命 | 275 / 7.5 秒 / 900 |
| 攻击周期 | 约 4.3 秒，本行有目标才起手 |
| 分裂 | 每轮在本行及有效的相邻行各发一箭，边界行不生成越界箭 |
| 穿透 | 每箭沿所在行向右飞行，每个经过的目标受到一次 390 点伤害 |
| 寒冰 | 每轮独立掷一次 20% 概率；该轮所有箭命中后施加现有寒冰减速 |

两种弹丸类型分别保存普通和寒冰属性；植物攻击进度沿用 `mShootingCounter`，弹丸飞行位置、类型和动画 attachment 沿用 `.v4` 现有字段。穿透按弹头从上帧到本帧扫过僵尸碰撞矩形左边界结算；同一刻重叠的多个僵尸逐一处理，不需要新增命中列表存档字段。

## 资源与检查

`scripts/gen-oak-archer-reanim.py` 锁定用户提供的 `OAKSHOOTER` PAM 哈希、标签及第 269/373 帧动作点，转换待机、两段普通攻击及水面动画。资源输出为本体、普通箭和寒冰箭的 `.reanim` 与所需 PNG；不读取或生成能量豆特效。

```bash
python3 scripts/gen-oak-archer-reanim.py --source /path/to/oakshooter --check
python3 tools/reanim-preview.py res/main/reanim/OakArcher.reanim /tmp/oak-idle.png 20 2
python3 tools/reanim-preview.py res/main/reanim/OakArcher.reanim /tmp/oak-attack.png 230 2
python3 tools/reanim-preview.py res/main/reanim/OakArcher.reanim /tmp/oak-attack2.png 340 2
cmake --build build
```

实机仍需检查三行轨迹、屋顶和泳池行高、贯穿及重叠僵尸、寒冰命中、卡面和本体落位，以及攻击中和弹丸飞行中保存／读取。离线预览与编译不能代替游戏内验收。
