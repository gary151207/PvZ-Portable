# 火焰机枪射手（Fire Gatling Pea）设计

**合成态**植物（`SeedType::SEED_FIRE_GATLING_PEA`，隐藏种子，**没有自己的种子卡**）：
把**火豌豆射手**卡（175 阳光）种在已种的**机枪射手**上升级即得 —— 与**寒冰机枪射手**
（寒冰射手 × 机枪射手）**完全同一条路径**。

- **普攻**：与机枪射手同一轮 **4 连发**（`mShootingCounter` 18/35/51/68 各一发），
  但每一发都是**紫火豌豆**（`PROJECTILE_PURPLE_FIRE_PEA`，与火豌豆射手同一颗子弹）。
- **大招（散射）**：触发/时长/概率成长/扇形弹道/伤害 **全部照抄机枪射手**，
  唯一区别是每颗子弹**各自**掷骰：**50% 紫火豌豆 / 50% 普通火豌豆**（`PROJECTILE_FIREBALL`）。

## 数值与节奏

| 项 | 值 | 位置 |
| --- | --- | --- |
| 阳光 / 冷却 | 175 / 750 厘秒（与火豌豆射手卡一致，合成不额外收费） | `gPlantDefs`（`src/Lawn/Plant.cpp`） |
| 生命 | 300（引擎统一值） | `Plant::PlantInitialize` |
| 发射节奏 | `mLaunchRate = 100`（与机枪射手完全一致） | `gPlantDefs` |
| 普攻连发 | 一轮 4 发，`mShootingCounter` = 18 / 35 / 51 / 68 | `Plant::UpdateShooting` |
| 普攻弹种 | 100% `PROJECTILE_PURPLE_FIRE_PEA`（直击 65 + 群伤 + 易伤 40%/4 秒） | `Plant::Fire` |
| 大招触发 | 每轮起手 `Rand(100) < mGatlingScatterChance` → `mGatlingScatterCountdown = 300` | `Plant::FindTargetAndFire` |
| 大招弹幕 | 每 2 帧 2 颗、±10°、`MOTION_STAR`、速度 3.33、`mDamageOverride = 200` | `Plant::Fire` 散射分支 |
| 大招弹种 | 每颗独立掷骰：50% 紫火豌豆 / 50% 火豌豆 | `FIRE_GATLING_SCATTER_PURPLE_PERCENT` |
| 概率成长 | 非散射期每发主子弹 50% 概率 `mGatlingScatterChance++`（上限 100，散射结束重置 3） | `Plant::Fire` |

阳光与冷却取自**被消耗的那张卡**（火豌豆射手，175 / 750），所以合成不需要额外规则：
`Board::MouseDownWithPlant` 照常按 `mCursorObject->mType` 扣 175 阳光、走火豌豆射手的卡槽冷却。
扣款之后会**原样返还 175**（见「机制零：合成返阳光」），净花费 0。

## 机制零：合成返阳光（寒冰 / 火焰机枪射手共用）

**合成机枪射手升级形态不花钱**：寒冰射手 @ 机枪射手 → 寒冰机枪射手、火豌豆射手 @ 机枪射手 →
火焰机枪射手，两种合成都把卡价 **`GATLING_SYNTHESIS_REFUND = 175`** 原样还给玩家（两张卡都是 175）。

实现照抄仓库里已有的「究极形态互换返阳光」（`ELECTRIC_STARFRUIT_SWITCH_REFUND`）：

1. 种卡照常走原流程扣款（`TakeSunMoney`，175）—— 保留了"阳光不够就种不下"与卡槽冷却的正常手感；
2. 两条合成分支各置一个 `aIsGatlingSynthesis = true`；
3. 在"原植物已 `Die()`"之后：`AddSunMoney(175)` + `FOLEY_SUN` + 提示字幕 `[GATLING_SYNTHESIS_REFUND]`
   （"合成完成，返还 175 阳光！"）。

**只在真的扣过款时返还**：原来的扣款条件是
`!mEasyPlantingCheat && mCursorType == CURSOR_TYPE_PLANT_FROM_BANK && !HasConveyorBeltSeedBank()`，
这里把它提成 `aPaysWithSun` 供两边共用 —— 传送带关卡与"免费种植"本来就不扣钱，无条件返还会**白送阳光**。

> 与究极形态互换的差别：那边是**固定**返还 225（不看你付了多少），这边按卡的实价
> `GATLING_SYNTHESIS_REFUND = 175` 返还，并且额外加了"扣过款才返"的守卫。

## 机制一：合成路径（照抄寒冰机枪射手）

三处改动，缺一不可：

1. `Plant::IsUpgradableTo`：`theUpgradedType == SEED_FIRE_PEASHOOTER && mSeedType == SEED_GATLINGPEA`
   → `true`。这让 `Board::CanPlantAt` 在机枪射手上返回 `PLANTING_OK`（卡片高亮、可落下），
   也让 `UpdateReanimColor` 的"可升级目标闪白"生效。
2. `Board::MouseDownWithPlant`：在"原植物 `Die()` / 扣阳光"之后把**实际要种的种子**
   （`aPlantSeedType`）改写成 `SEED_FIRE_GATLING_PEA`，`aPlantImitaterType` 清成 `SEED_NONE`。
3. `gPlantDefs` 注册 `SEED_FIRE_GATLING_PEA` 一行（`GetPlantDefinition` 的断言要求表按枚举序对齐）。

> 为什么**不**做成"升级卡"（`Plant::IsUpgrade`）：合成态没有自己的卡，`IsUpgrade` 会让
> 选卡器要求同选底座、并弹 `PLANTING_NEEDS_UPGRADE` 提示。寒冰机枪射手同样不在 `IsUpgrade` 里。

## 机制二：普攻 4 发紫火豌豆

`Plant::FindTargetAndFire` / `Plant::UpdateShooting` 里三处"机枪射手家族"列表
（`SEED_GATLINGPEA` / `SEED_ELECTRIC_GATLING_PEA` / `SEED_SNOW_GATLING_PEA`）都并入本类型，
于是**头部开火动画节奏**（`mShootingCounter = 100`、`mAnimRate = 38`）与**4 连发出弹帧**
自动与机枪射手一致，不需要复制任何逻辑。

弹种在 `Plant::Fire` 的 `switch (mSeedType)` 里与 `SEED_FIRE_PEASHOOTER` **共用同一个 case**：
两者都是 `PROJECTILE_PURPLE_FIRE_PEA`。

**不做**机枪射手那 3% 电能豌豆掷骰：`RollGatlingBulletType` 只认 `SEED_GATLINGPEA`，
火焰机枪射手天然打不出电能豌豆。

## 机制三：大招 50% 紫火 / 50% 火豌豆

散射分支（`mGatlingScatterCountdown > 0`）里，每颗子弹的弹种改走

```cpp
auto RollGatlingScatterType = [this, &RollGatlingBulletType](ProjectileType theBaseType) -> ProjectileType
{
    if (mSeedType == SeedType::SEED_FIRE_GATLING_PEA)
    {
        return Rand(100) < FIRE_GATLING_SCATTER_PURPLE_PERCENT
            ? ProjectileType::PROJECTILE_PURPLE_FIRE_PEA
            : ProjectileType::PROJECTILE_FIREBALL;
    }
    return RollGatlingBulletType(theBaseType);
};
```

- 其余三只机枪（普通 / 究极电能 / 寒冰）走 `RollGatlingBulletType`，行为**逐字节不变**
  （普通机枪的 3% 电能豌豆仍然生效）。
- `mDamageOverride = 200` **保持**：这是"大招机制与机枪射手一样"的一部分
  （散射期间的伤害口径就是 200，紫火豌豆的 65 / 火豌豆的 40 都被它盖住）。
- 紫火豌豆的**命中易伤**依旧生效（`Projectile::DoImpact` 里与伤害口径无关，照常挂 4 秒标记）。
- 散射期间**不播放** `FOLEY_THROW`（与其它三只机枪一致：散射分支不进那段音效代码）。

### 关键坑：直接生成的 `PROJECTILE_FIREBALL` 原本没有视觉

上面这条掷骰一开始"看起来失效"：进游戏开大只看得见**紫**火，像是 100% 紫火豌豆。原因是原版
**从没有人直接生成过火球弹丸** —— 普通火豌豆只由 `Plant::UpdateTorchwood()` 在豌豆飞过火炬树桩时
调用 `Projectile::ConvertToFireball()` **事后补挂**火焰动画；因此 `ProjectileInitialize()` 的
`PROJECTILE_FIREBALL` 那一支原本写的是 `TOD_ASSERT(false)`（Release 下断言被编掉 → **什么都不做**）。

于是 `AddProjectile(..., PROJECTILE_FIREBALL)` 出来的弹丸：
- 伤害/溅射/去冰冻全部正常（`IsSplashDamage` / `GetDamageType` 等只认类型），
- 但 `Projectile::Draw()` 对火球是 `aImage = nullptr`（视觉全靠附属动画），而附属动画根本没人挂
  → **一颗只有影子、看不见火球的隐形弹丸**。

两发里有一发隐形，玩家看到的就是"大招只出紫火"。修法：在 `ProjectileInitialize()` 的
`PROJECTILE_FIREBALL` 分支里补齐与紫火豌豆**完全同款**、只是**不带紫火滤镜**的橙红火球动画
（`REANIM_FIRE_PEA` + `AttachReanim`，与 `ConvertToFireball()` 那段一致）。
`ConvertToFireball()` 自身保持不变（它是在已有豌豆上补挂，额外的 `MOTION_BACKWARDS` 翻转仍归它管），
且 `UpdateTorchwood()` 只转换 `PROJECTILE_PEA` / `PROJECTILE_SNOWPEA`，所以已点着的火球不会被二次挂图。

## 机制四：贴图接入

新增 `ReanimationType::REANIM_FIRE_GATLINGPEA`：与机枪射手**同一个 reanim 文件**
（`reanim/GatlingPea.reanim`）、**独立的定义槽**、**`REANIM_NO_ATLAS`**
（不建自己的 Atlas → 定义里的 `mImage` 始终是真实指针，换图不挑时机，也不与机枪射手的 Atlas 共用源贴图）。

`FireGatlingHasCustomArt()` 用与寒冰机枪射手**同一个** `ApplyReanimArtSwaps()` 助手逐帧替换：

| 定义里的原图 | 换成 |
| --- | --- |
| `reanim/GATLINGPEA_HEAD` | `reanim/FireGatling_head.png` |
| `reanim/GATLINGPEA_MOUTH` | `reanim/FireGatling_mouth.png` |
| `reanim/GATLINGPEA_MOUTH_OVERLAY` | `reanim/FireGatling_mouth_overlay.png` |
| `reanim/GATLINGPEA_BLINK1` | `reanim/FireGatling_blink1.png` |
| `reanim/GATLINGPEA_BLINK2` | `reanim/FireGatling_blink2.png` |
| `reanim/GATLINGPEA_HELMET` | `reanim/FireGatling_helmet.png` |

**枪管（`GATLINGPEA_BARREL`）不替换**：素材里没有 `FireGatling_barrel.png`，枪管继续用
原版机枪射手的绿色枪管。这也正好绕开 `ApplyReanimArtSwaps()` 的"**缺一张就整体回退**"语义 ——
只列出确实存在的那六张，函数就会成功返回，而不是因为缺枪管把整套贴图丢掉。

安全阀与既有专用贴图完全一致：每张图必须**带透明通道**、且**尺寸与原图逐像素一致**
（reanim 按帧号索引贴图列，尺寸不符会串帧）；任何一张不合格 → 整体回退到普通机枪射手外观。
`blink1/blink2` 必须**按原图指针逐帧替换**（`anim_blink` 与 `idle_shoot_blink` 各引用两张），
不能挂 `mImageOverride`（一条轨道一张图会把眨眼压成同一张）。

调用点（与寒冰/电能机枪射手一一对应）：

- `Plant::PlantInitialize` → 建 body/head 动画**之前**解析实际 reanim 类型；
- `Plant::PreloadPlantResources` → 提前装载定义；
- `Plant::AttachBlinkAnim` → 眨眼附件要用同一个 reanim 类型（否则眨眼是绿色眼睛）；
- `ReanimatorCache::DrawReanimatorFrame` → 卡面/图鉴/光标预览的临时实例；
- `ReanimatorCache::MakeCachedPlantFrame` → 缓存帧额外画 `anim_head_idle` 一层。

## 接线清单

| 文件 | 改动 |
| --- | --- |
| `src/ConstEnums.h` | `SEED_FIRE_GATLING_PEA`（`SEED_FIRE_PEASHOOTER` 之后、`NUM_SEED_TYPES` 之前）；`REANIM_FIRE_GATLINGPEA`（`REANIM_FIRE_PEASHOOTER` 之后、`NUM_REANIMS` 之前） |
| `src/Sexy.TodLib/Reanimator.cpp` | 定义槽 → `reanim/GatlingPea.reanim` + `REANIM_NO_ATLAS` |
| `src/GameConstants.h` | `FIRE_GATLING_SCATTER_PURPLE_PERCENT = 50`；`GATLING_SYNTHESIS_REFUND = 175`（两种机枪合成共用） |
| `src/Lawn/Plant.h` | `FireGatlingHasCustomArt()` / `FireGatlingUsesCustomArt()` / `FireGatlingReanimType()` |
| `src/Lawn/Plant.cpp` | 专用贴图三件套；`gPlantDefs` 行；`PlantInitialize`（reanim 解析 + 豌豆家族 case）；`PreloadPlantResources`；`AttachBlinkAnim`（类型 + 组列表）；`FindTargetAndFire` / `UpdateShooting` / `Fire` 各机枪列表 + 散射弹种掷骰 + 枪口坐标 + 孤狼伤害；`IsUpgradableTo` 合成规则 |
| `src/Lawn/Board.cpp` | 合成改写种子 + **合成返阳光**（`aPaysWithSun` / `aIsGatlingSynthesis` / `AddSunMoney(GATLING_SYNTHESIS_REFUND)`）；HP 悬浮提示的散射状态；冰关沙盒旅行页 |
| `src/Lawn/Projectile.cpp` | `ProjectileInitialize()` 的 `PROJECTILE_FIREBALL` 分支补齐普通火豌豆的火球动画（原本只有一句断言 → 直接生成的普通火豌豆没有视觉） |
| `src/Lawn/System/ReanimationLawn.cpp` | 卡面/图鉴/光标预览的 reanim 类型与贴图预换、缓存帧头部层 |
| `src/Lawn/Widget/AlmanacDialog.h/.cpp` | 图鉴植物页第 2 页（第 8 个位置，正好填满第一行 8 列） |
| `res/properties/pvzp-strings.xml`、`.zh-CN.xml` | `FIRE_GATLING_PEA` / `_TOOLTIP` / `_DESCRIPTION`；火豌豆射手 tooltip 补一句"种在机枪射手上可合成" |
| `scripts/check-fire-gatling-pea.ps1` | 源级检查 |

### 顺带修好的旧检查脚本

本类型往"机枪射手家族"的共享列表里多加了一个成员，因此 `check-electric-gatling-pea.ps1`、
`check-fire-pea-shooter.ps1`、`check-lone-wolf-attack.ps1` 里那些"**紧邻**下一项"的正则
（`SEED_ELECTRIC_GATLING_PEA\)` 之类）会因为中间多了 `SEED_FIRE_GATLING_PEA` 而失配 ——
已把这些间隔放宽（语义不变：仍然要求该类型出现在同一分支/同一组里）。
同时顺手修掉了几处**早在本改动之前就已经失效**的旧断言：

- `check-fire-pea-shooter.ps1`：`NUM_ALMANAC_EXTRA_SEEDS` 不再写死，改成"必须等于 `gAlmanacExtraSeeds`
  的实际条目数"；`MakeCachedPlantFrame` 的头部层列表断言放宽。
- `check-electric-gatling-pea.ps1` / `check-electric-starfruit.ps1` / `check-peater-1-5.ps1`：
  旧的 `properties/pvzp-strings.xml` 路径（资源已搬到 `res/` 下）、`NUM_PROJECTILES` 仍是 16、
  `ELECTRIC_STAR_LINGER_TICKS` 仍是 500、注释里还写着"究极电能杨桃"等**历史遗留**断言，
  以及同样的正则紧邻问题。
- `check-huge-wave-text-utf8.ps1`：找 `void TodDrawStringMatrix` 函数体用的收尾模式是 `\n\}\n`，
  只认 LF；本仓库的源文件是 CRLF，于是永远找不到 → 改成 `\r?\n\}`。

改完 `scripts/check-*.ps1` 24 个脚本全绿。

## 明确不做

- **不**新增弹丸类型：紫火豌豆与火豌豆都是既有 `ProjectileType`。
- **不**改机枪射手的任何既有行为（`mDamageOverride = 200`、概率成长、扇形参数一个字没动）。
- **不**做电能豌豆掷骰（`RollGatlingBulletType` 只认 `SEED_GATLINGPEA`）。
- **不**给合成态自己的种子卡/选卡器条目（`gTravelPlantDefs`、`IsUpgrade`、`IsRedCard` 都不加）。
- **不**替换枪管贴图（素材没有；见上）。
- **不**动存档格式：`mSeedType` 是既有字段，新值落在 `NUM_SEED_TYPES` 之前，旧档读入不受影响；
  `mGatlingScatterCountdown` 等 mod 计时器本来就不入档。

## 测试

1. **合成**：旅行关选卡带上**机枪射手**与**火豌豆射手** → 先种机枪射手 → 把火豌豆射手卡拖到它上面
   → 卡片应高亮（目标格闪白）、落地扣 175 阳光、原地变成**火焰机枪射手**（橙色头 + 头盔 + 火焰五官，
   枪管仍是原版绿色）。
   **阳光要原样还回来**：合成瞬间阳光总数不变，并播一次收阳光音效、飘出"合成完成，返还 175 阳光！"。
   用**寒冰射手** @ 机枪射手 验证同一件事（寒冰机枪射手也有返还）。
   两个反例：**传送带关卡**与**免费种植**下不返（本来就没扣钱）。
2. **普攻**：对着僵尸观察一轮 4 连发 —— 4 发全是**紫色**火球；命中的僵尸**变红 4 秒**；
   相邻行/命中点周围的僵尸吃到 1/3 溅射伤害。
3. **大招**：连续开火把 `mGatlingScatterChance` 养起来（HP 悬浮提示会显示 `Scatter: x%/shot`），
   开大后观察 3 秒散射 —— 子弹应为**紫色与橙色混排**，肉眼大致各半。
   > 这一条曾经"看起来不成立"（只出紫火），根因是普通火豌豆弹丸没有视觉 —— 见上面
   > 「关键坑：直接生成的 `PROJECTILE_FIREBALL` 原本没有视觉」。若改完仍只看到一种颜色，
   > 先确认 `ProjectileInitialize()` 的 `PROJECTILE_FIREBALL` 分支里确实挂上了 `REANIM_FIRE_PEA`。
4. **贴图安全阀**：临时把 `res/main/reanim/FireGatling_head.png` 改名 → 整株应回退成
   **普通机枪射手**（而不是画出白方块或缺图崩溃）；改回来即恢复。
5. **眨眼**：放置一段时间后眨眼应当是**橙色版**的眼睛/眼皮（不是绿色）。
6. **回归**：普通机枪射手（4 发豌豆 / 大招散射 / 3% 电能豌豆）、究极电能机枪射手、寒冰机枪射手
   行为与贴图不变；火豌豆射手单独种下时的行为不变。
7. **图鉴**：植物页第 2 页第 8 格是火焰机枪射手，点开有名字/描述/数值，预览图是橙色版。
8. **存档**：种下火焰机枪射手后存档 / 读档，植物应仍是火焰机枪射手（`mSeedType` 落档）。
