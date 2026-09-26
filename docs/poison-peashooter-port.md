# 一阶毒液豌豆射手移植复盘

**范围：** 国服《植物大战僵尸 2》的 `PoisonPeashooter`，仅一阶普通攻击  
**入口：** 仅斗蛐蛐 2 第二选卡页，排在孢子菇之后  
**实机反馈日期：** 2026-09-26  
**通用流程：** [PvZ2 植物原生移植指南](pvz2-plant-porting-guide.md)

本文记录最终实现及调试中实际遇到的问题，供下一株 PvZ2 植物移植时对照。
不接入装扮、二阶及以上机制、能量豆效果和能量豆地块效果。

## 1. 已锁定的行为口径

| 项目 | 本项目实现 |
| --- | --- |
| 费用、冷却、生命 | 175 阳光、500 tick（5 秒）、300 点生命 |
| 普攻 | 同行单目标直线弹；每轮 150 tick 减去 0–14 tick 随机偏移，发射一颗 |
| 直击 | 10 点，走现有头盔、护盾伤害规则 |
| 毒伤 | 命中加一层，最多五层；首次命中后每 100 tick 结算一次，每层 50 点，作用于本体生命 |
| 持续与续时 | 命中刷新 500 tick；重复命中不重置已有的毒伤结算节奏 |
| 减速 | 中毒时移动与动画按 50% 处理；与寒冰同时存在时，移动取寒冰的 40% 系数 |
| 提示 | 中毒期间僵尸本体泛紫色，兼有寒冰时呈蓝紫色；调试状态文字显示层数和剩余时间 |
| 存档 | 僵尸 `.v4` TLV 字段 3 保存层数、剩余时间和下次结算计时 |

这些战斗数值按本次用户选定的国服一阶口径实现。所给提取目录包含动画和图片，
没有可直接核验这些数值的 APK 配置；**50% 减速仍未由 APK 配置核实**。

## 2. 素材转换与几何校准

源素材位于仓库外的 `poisonpeashooter` 提取目录。转换器
`scripts/gen-poison-peashooter-reanim.py` 复用孢子菇 PAM 展开代码，固定植物和弹丸
PAM JSON 的 SHA-256、版本、30 FPS、label 顺序与第 155 帧的 `use_action`。
它逐帧展开 display list 和嵌套 sprite，过滤装扮与高阶层，只输出两份 `.reanim`
及 51 张一级素材 PNG。运行时使用仓库内的生成文件，不读取提取目录。

| 资源 | 保留的段 | 转换参数 |
| --- | --- | --- |
| `PoisonPeashooter.reanim` | `anim_idle`、`anim_idle2`、`anim_idle3`、`anim_shooting` | 缩放 `0.55`，原点 `(112, 110)` |
| `PoisonPeashooterProjectile.reanim` | 一级 `anim_fly`、`anim_hit` | 缩放 `0.78`，原点 `(176, 195)` |

弹丸比例以**圆形主体**为基准：转换后的可见高度约 24 px，与项目普通豌豆的
24 px 主体相近；拖尾保留原形。卡面另在 `SeedPacket.cpp` 使用 `0.48` 缩放和
`(4, 8)` 偏移。卡面、本体、弹丸、植物影子、弹丸影子各有坐标系，不能靠调整
其中一个偏移修复所有位置问题。

生成、逐字节检查及抽帧预览：

```bash
python3 scripts/gen-poison-peashooter-reanim.py --source /path/to/poisonpeashooter
python3 scripts/gen-poison-peashooter-reanim.py --source /path/to/poisonpeashooter --check
python3 tools/reanim-preview.py res/main/reanim/PoisonPeashooter.reanim /tmp/poison-idle.png 15 2
python3 tools/reanim-preview.py res/main/reanim/PoisonPeashooter.reanim /tmp/poison-attack.png 155 2
python3 tools/reanim-preview.py res/main/reanim/PoisonPeashooterProjectile.reanim /tmp/poison-fly.png 3 3
python3 tools/reanim-preview.py res/main/reanim/PoisonPeashooterProjectile.reanim /tmp/poison-hit.png 12 3
```

`--check` 需要能访问源目录；在另一台机器上应显式传入 `--source`。预览静态帧
只能检查资源和相对位置，完整性、拖尾、影子与碰撞仍需运行游戏观察。

## 3. 原生逻辑接线

遵循项目的枚举分支架构，不新建植物或僵尸子类，也不改旧枚举值。

| 职责 | 接线位置与要点 |
| --- | --- |
| 类型、数值、动画 | `ConstEnums.h` 追加枚举；`Plant.cpp`、`Projectile.cpp` 定义表；`Reanimator.cpp` 注册 `REANIM_NO_ATLAS`；`Plant::PreloadPlantResources` 预加载弹丸 |
| 起手与出弹 | `Plant::FindTargetAndFire` 播放本体 `anim_shooting`；`UpdateShooting` 在动作点调用一次 `Fire`，随后退出毒液豌豆专用分支 |
| 弹丸 | `Plant::Fire` 生成 `PROJECTILE_POISON_PEA`，枪口为植物逻辑坐标 `(mX + 101, mY + 35)`；`Projectile` 管理直飞、碰撞、一级飞行和命中动画 |
| 中毒 | `Projectile::DoImpact` 先结算直击，再对仍存活的目标调用 `Zombie::ApplyPoisonPea`；`Zombie::UpdatePoisonPea` 结算持续伤害和到期清除 |
| 状态表现 | `Zombie::GetMovementSlowFactor`、动画速度、`Zombie::DrawReanim` 的颜色覆写；弹道预判使用实际减速结果 |
| 产品入口 | `LawnApp::HasSeedType` 限定模式；`SeedChooserScreen` 控制第二页及排位；`AlmanacDialog` 加图鉴卡位；两份 `pvzp-strings*.xml` 加名称、提示和说明 |
| 存档 | `SaveGame.cpp` 为僵尸增加可选 TLV 字段 3；旧 `.v4` 缺少字段时沿用初始化的零值 |

植物 PAM 攻击段为第 144–191 帧，`use_action` 在第 155 帧，即段内约第 11 帧。
游戏逻辑为 100 Hz，攻击动画以 35 FPS 播放：整段约 `48 / 35 × 100 ≈ 137`
tick，动作点约 `11 / 35 × 100 ≈ 31` tick。因此起手置
`mShootingCounter = 137`，自减到 `106` 时发射。**专用分支必须返回**：
若继续落入 `UpdateShooting` 的通用 `mShootingCounter == 1` 分支，同一轮会多发一颗。

毒伤调用 `TakeBodyDamage`，不经头盔或护盾；直击仍按普通弹丸规则结算。
首层命中建立 100 tick 结算节奏，此后只增加层数、刷新 500 tick 时限。
中毒结束或僵尸死亡后清除状态；画面颜色直接由剩余时限决定，读档恢复状态后
也会恢复对应着色。现有烧毁、魅惑、火豌豆易伤等更高优先级颜色仍按原分支处理。

## 4. 实机反馈与修复经验

| 现象 | 原因与最终处理 | 下次移植先检查 |
| --- | --- | --- |
| 卡牌图像偏到角落，种下后本体也不居中 | PAM 本体原点与项目卡片、草坪坐标不同。调整转换器的本体比例/原点，卡面再单独调缩放和偏移 | 分别预览卡面与战场，不共用一组偏移 |
| 本体脚下没有影子 | 本体贴图和植物逻辑影子没有对齐；校准本体原点，使可见脚部落在影子处 | 先确认影子是否处于正确格位，再调本体画面 |
| 弹丸发出后很快消失 | 低枪口使弹丸与行影子的初始高度差小于豌豆默认的 28 px 触地阈值。给毒液豌豆单独使用 18 px 阈值 | 同时检查出弹高度、`mShadowY - mPosY` 与 `CheckForHighGround` |
| 弹丸偏下或与影子错位 | PAM 特效原点、枪口坐标与影子位置不一致。联动校准弹丸原点和 `(mX + 101, mY + 35)` | 视觉位置、碰撞位置、影子位置分别检查 |
| 弹丸比普通豌豆小 | 最初按整张带拖尾图片估计比例。改按圆形主体直径对照普通豌豆，最终比例 `0.78` | 比较主体，不把拖尾算进直径 |
| 飞行中圆头时有时无，看起来像被裁切 | PAM 每帧切换 display-list 轨道；默认消失帧截断过早隐藏旧轨道。仅对该弹丸飞行和命中 reanim 关闭截断 | 连续观察多个动画帧，静态单帧不足以发现此问题 |
| 一次攻击出现两颗弹丸 | 动作点出弹后又触发通用计数为 1 的出弹路径。毒液豌豆专用分支处理完立即返回 | 审核整个 `UpdateShooting`，计算一轮内所有可能的 `Fire` 路径 |
| 图鉴名称缺字、macOS 回退字形倒置 | 新文案包含资源字体没有的字。接入 CoreText 字形回退后，确认其位图缓冲区行序与 `MemoryImage` 一致，不能额外翻转 | 同时检查本地化键、资源字体字库、平台回退和实际显示方向 |
| 僵尸中毒但外观无变化 | 战斗状态已接入，渲染状态遗漏。在 `Zombie::DrawReanim` 增加紫色覆写；与寒冰并存时用蓝紫色 | 新增状态需检查逻辑、动画速度、渲染、存档四条路径 |

右侧“缺口”曾容易被误判为 PNG 裁边；原 PNG 完整，实际是动态轨道显隐问题。
同理，单张截图里有两颗弹丸，不足以判断是否同一轮攻击；需要追踪计数器和
所有 `Fire` 调用点。本案例最终定位到了动作点和通用收尾出弹的叠加。

## 5. 验证记录与后续检查

移植过程中运行过转换器 `--check`、CMake 构建及 `git diff --check`。
用户随后在游戏中确认：弹丸飞行画面的缺失问题已解决、重复出弹已修复、
中毒紫色视觉效果符合预期。卡面、种植位置、影子和弹丸大小/高度根据运行截图
逐轮调校。上述反馈不能代替对所有僵尸、场景和存档的系统测试。

下一株带持续状态的植物，应按以下顺序验收：

1. 源哈希和输出清单、枚举整数不变、资源注册及完整构建。
2. 离线预览待机、攻击动作点、连续飞行帧和命中帧；实机比较卡面、本体、影子和枪口。
3. 单株单目标计数，核对每轮出弹次数、飞行碰撞和命中次数。
4. 分别检查 1–5 层毒伤、续时、护甲穿透、减速、寒冰并存、死亡清除和着色恢复。
5. 中毒期间保存/读取，再读取缺少新字段的旧 `.v4` 存档。
6. 验证目标模式第二页可选，第一页及其它模式不可选；检查图鉴和双份中文文案。

第 4–5 项及所有模式隔离场景在本次对话中没有完整的实机验收记录；后续修改
相关逻辑时仍应执行这些检查。
