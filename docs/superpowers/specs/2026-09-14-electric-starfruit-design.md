# 设计：究极电能杨桃（Electric Starfruit）— 旅行红卡 + 升级卡 + 究极形态互换

日期：2026-09-14
状态：已批准（2026-09-14 与用户确认获取方式、互换规则）

用户原始需求：

> 制作新植物：究极电能杨桃；效果为5颗追踪的电能星星然后呢追踪到僵尸，命中后会在该僵尸那里存在5秒，
> 期间每一游戏刻对僵尸造成30点伤害，贴图已放在main.pak里，其余效果跟杨桃一致

设计澄清（用户拍板）：

| 议题 | 结论 |
| --- | --- |
| 获取范围 | **仅旅行模式**（与 `SEED_ELECTRIC_GATLING_PEA` 同档的红卡） |
| 卡片类型 | **红卡 + 升级卡（紫卡语义）**：拖到已种的**杨桃**上原地升级 |
| 升级价格 | **300 阳光** |
| 冷却 | **30 秒**（`mRefreshTime = 3000` = 30.01 s，与机枪射手/究极电能机枪射手同档） |
| 与究极电能机枪射手互换 | **拖普通杨桃卡（125 阳光）到究极电能机枪射手 → 变成究极电能杨桃，返还 225 阳光** |
| 反向互换 | **拖机枪射手卡到究极电能杨桃 → 变回究极电能机枪射手，返还 225 阳光** |
| 星星数量/方向 | **5 颗**，方向与普通杨桃完全一致（左/上/下/右上/右下 30°） |
| 追踪 | 与普通杨桃**同一套**追踪逻辑（飞行 1 格后锁定最靠左的僵尸） |
| 命中效果 | 星星**不消失**，钉在该僵尸身上 **2.5 秒**，期间**每 0.15 秒造成 30 点电能伤害** |
| 目标死亡后 | **自动改追下一个目标**（改追期间 2.5 秒总寿命照常流逝，见下"弹丸状态机"） |
| 其余机制 | 与杨桃完全一致（125 阳光本体数值、300 生命、100 发射节奏、`anim_shoot`、护盾穿透、无影子、光照偏移 +10） |
| 贴图 | 用玩家自备的 `reanim/Electric_Starfruit_body/eyes1/eyes2.png`（main.pak 内已有）；叶/茎/嘴/眉保持原版 |

> **数值调整（2026-09-14 后续需求）**：电能伤害不再是"每游戏刻（10 ms）一次"，
> 而是**每 0.15 秒一次**（`ELECTRIC_DAMAGE_INTERVAL_TICKS = 15` 刻）；钉住总时长
> 从 5 秒改为 **2.5 秒**（`ELECTRIC_STAR_LINGER_TICKS = 250`）。
> 同一节奏也适用于**电能豌豆**（`PROJECTILE_FIREPEA_RED`，究极电能机枪射手的子弹）。
> 实现：豌豆在 `CheckForCollision` 里以 `mProjectileAge % ELECTRIC_DAMAGE_INTERVAL_TICKS` 结算；
> 星星用 `Projectile::mElectricDamageCountdown`（每帧在 `Projectile::Update` 自减，
> 归零才 `TakeDamage` 并重置为 15）。下文各节中"每游戏刻 / 5 秒 / 500"的表述均以此调整为准。

## 背景与目标

旅行模式子系统（`src/Lawn/Travel.h/.cpp`）已落地，旅行专属植物现有四只：
`SEED_FUMESHROOM_GROUP`（紫卡）、`SEED_GIANT_WALLNUT`（红卡）、`SEED_PEATER_1_5`（红卡）、
`SEED_ELECTRIC_GATLING_PEA`（红卡 + 升级卡）。本次新增第五只 `SEED_ELECTRIC_STARFRUIT`，
并把"究极形态"从单点扩展成**一对可互换的究极植物**。

1. **纯新增枚举** `SeedType::SEED_ELECTRIC_STARFRUIT`，前插在 `SEED_ELECTRIC_GATLING_PEA` 之后、
   `NUM_SEED_TYPES` 之前，**不动任何既有枚举值** → 存档稳定。
2. **升级关系**：种植前提 = 场上有**杨桃**（`SEED_STARFRUIT`），拖到它身上原地升级（走引擎既有
   升级卡路径 `Board::MouseDownWithPlant`）。
3. **互换关系**：见下节"究极形态互换"。
4. **弹丸**：新增 `PROJECTILE_ELECTRIC_STAR`（唯一的新弹丸类型），飞行段复用杨桃的
   `MOTION_STAR` 追踪，命中后进入"钉住"状态。
5. **贴图**：新增 `ReanimationType::REANIM_ELECTRIC_STARFRUIT`，加载与杨桃**同一个**
   `reanim/Starfruit.reanim`，装载后把 `body / eyes1 / eyes2` 三张图换成电能版。

**非目标**：
- 不改普通杨桃（`SEED_STARFRUIT`）的任何行为与贴图。
- 不改究极电能机枪射手的数值（只是多了一条"被机枪射手卡互换"的入口）。
- 不新增美术 / 不重打包 `main.pak`（专用贴图由玩家提供，缺失时回退到电能蓝染色）。
- 不动 TRAVEL_1 / TRAVEL_2 传送带阵容（只走选卡器页 1 + 冰冻沙盒旅行页）。
- 不做自动化测试（仓库无测试框架），以编译 + 源级检查 + 手动试玩验证。

## 现状调研

### 杨桃的发射链路（本次全部照抄）

| 环节 | 位置 | 行为 |
| --- | --- | --- |
| 攻击节奏 | `Plant::UpdateShooter`（Plant.cpp ~L1158） | `mLaunchCounter` 到点后 `LaunchStarFruit()` |
| 索敌 | `Plant::FindStarFruitTarget`（~L1077） | 任意一个"可被本武器伤害"的僵尸即开火（本仓库已简化成整场索敌） |
| 出手 | `Plant::LaunchStarFruit`（~L1094） | 播 `anim_shoot`，`mShootingCounter = 40` |
| 实际发射 | `Plant::StarFruitFire`（~L1103） | 5 颗 `PROJECTILE_STAR` + `MOTION_STAR`，速度 3.33，方向 ±30° / 正上下 / 正左 |
| 命中后 | `Plant::Fire` 的 `SEED_STARFRUIT` 分支（~L5272） | 调 `StarFruitFire()` |

### 杨桃的追踪（本仓库既有魔改，本次直接复用）

`Projectile::UpdateNormalMotion`（Projectile.cpp ~L833）的 `MOTION_STAR` 分支：

- `mProjectileAge >= 24`（飞行约 1 格）后锁定**最靠左**（`mX` 最小）且可被伤害的僵尸，
  写进 `mTargetZombieID`；目标失效则重新索敌（矿工僵尸**还在钻地**时只有向左飞的星星能锁，出土后 5 颗一律正常索敌）。
- 锁定后每帧把 `mVelX/mVelY` 重定向到目标中心，速度仍为 3.33 → 形成"追踪"。
- `mTargetZombieID != NULL` 时 `GetDamageFlags()` 追加 `DAMAGE_BYPASSES_SHIELD`（与向左星一致）。
- 命中判定在 `Projectile::CheckForCollision`（~L367）：与目标矩形重叠且 `mPosY` 落在僵尸身内
  才 `DoImpact`。

> **结论**：究极电能杨桃的"5 颗追踪星星"= 复用上面整套逻辑，只把弹丸类型换成
> `PROJECTILE_ELECTRIC_STAR`，并在"命中"处改成进入钉住状态而不是 `DoImpact`。

**为什么 5 颗会一起追同一个僵尸**：既有索敌是"最靠左的可伤害僵尸"，5 颗各自独立求值，
因此通常收敛到同一目标（贴在同一僵尸上叠 5 层）。这与"其余效果跟杨桃一致"一致；
若将来想改成"一颗一个"，只需把索敌改成按 `mLastHitZombieID` 去重即可。

### 电能伤害的既有口径（本次沿用）

`PROJECTILE_FIREPEA_RED`（电能豌豆）在 `CheckForCollision` 里**每 tick（10 ms）**对矩形重叠的
每个僵尸造成 30 点伤害、无限穿透（Projectile.cpp ~L444）。用户说的"每一游戏刻 30 点伤害"
就是这套口径，因此本次的直接伤害常量与它对齐（`ELECTRIC_STAR_HIT_DAMAGE = 30`）。

### Starfruit.reanim 的轨道结构（决定换哪几张图）

从运行目录 `main.pak` 解出的 `reanim/Starfruit.reanim`（fps=12，38 帧时间轴）：

| 轨道 | 贴图 | 本次处理 |
| --- | --- | --- |
| `anim_face` | `IMAGE_REANIM_STARFRUIT_BODY` | **换成 `reanim/Electric_Starfruit_body.png`** |
| `Starfruit_eyes` | `IMAGE_REANIM_STARFRUIT_EYES1` / `_EYES2`（同轨两图换帧眨眼） | **换成 `reanim/Electric_Starfruit_eyes1/eyes2.png`** |
| `Starfruit_leaf` / `Starfruit_stem` | 叶 / 茎 | 不变（保持绿色） |
| `Starfruit_mouth` / `Starfruit_smile` | 嘴 / 笑 | 不变 |
| `Starfruit_eyebrow` | 眉 | 不变（杨桃本体 reanim 未引用，pak 里另有该图） |
| `anim_blink` / `anim_idle` / `anim_shoot` | 控制层，无贴图 | 不变 |

玩家提供的三张图实测：

| 文件 | 尺寸 | 透明通道 | 不透明像素均值 |
| --- | --- | --- | --- |
| `reanim/Electric_Starfruit_body.png` | 104×88（= `Starfruit_body.png`） | alpha 0–255 ✔ | (195,230,229) 电光蓝白 |
| `reanim/Electric_Starfruit_eyes1.png` | 35×20（= `Starfruit_eyes1.png`） | alpha 0–255 ✔ | (212,249,247) |
| `reanim/Electric_Starfruit_eyes2.png` | 35×20（= `Starfruit_eyes2.png`） | alpha 0–255 ✔ | (218,251,250) |

尺寸全部与同名原图一致、且自带透明通道 → 可直接走"换贴图"路径（校验逻辑与究极电能机枪射手
完全一致：**缺一张 / 不透明 / 尺寸不符 → 整体回退**）。

### 为什么杨桃的贴图接入比机枪射手简单

杨桃在 `PlantInitialize` 里**只创建一个实例**（`mBodyReanimID`，播 `anim_idle`）：头/脸/五官/
叶/茎/眼都在**同一个 reanim** 里，不像豌豆射手那样分 body / head 两个实例。因此：

- 只需要一个 `ReanimationType::REANIM_ELECTRIC_STARFRUIT`，不需要处理 attachment 颜色传播；
- 兜底染色直接作用在这唯一实例上（不需要 `mIgnoreExtraOverlayColor` 之类的逐轨豁免）；
- `AttachBlinkAnim` 里杨桃走的是"身体自身 `anim_blink`"路径，只需保证眨眼用的 reanim 类型
  与身体一致。

## 究极形态互换（新机制）

用户需求：**两只究极植物可以互相切换**，切换时"返还 225 阳光"。

| 手上拖的卡 | 落点已有植物 | 结果 | 阳光 |
| --- | --- | --- | --- |
| **杨桃**（125） | 究极电能机枪射手 | 究极电能杨桃 | 先扣 125，再**返还 225**（净 +100） |
| **机枪射手**（250） | 究极电能杨桃 | 究极电能机枪射手 | 先扣 250，再**返还 225**（净 −25） |
| 究极电能杨桃（300） | 杨桃 | 究极电能杨桃 | 扣 300（普通升级路径） |

实现方式（复用既有升级卡路径，不新开分支函数）：

1. `Plant::IsUpgradableTo` 追加两条：
   - `theUpgradedType == SEED_STARFRUIT && mSeedType == SEED_ELECTRIC_GATLING_PEA`
   - `theUpgradedType == SEED_GATLINGPEA && mSeedType == SEED_ELECTRIC_STARFRUIT`
   → `Board::CanPlantAt` 的"一般紫卡植物的更迭判断"分支自动放行落点（同时让手持卡片时的
   目标高亮 `Plant::IsPartOfUpgradableTo` 一并生效）。
2. `Board::MouseDownWithPlant` 在"销毁原植物"之后、`AddPlant` 之前，识别互换对并**改写要种的种子**：
   `SEED_STARFRUIT@究极电能机枪射手 → SEED_ELECTRIC_STARFRUIT`、
   `SEED_GATLINGPEA@究极电能杨桃 → SEED_ELECTRIC_GATLING_PEA`，然后 `Board::AddSunMoney(225)`
   返还阳光并播 `FOLEY_SUN` 提示。
3. `Board::PlantingRequirementsMet`：
   - 新增 `SEED_ELECTRIC_STARFRUIT` → 需要场上有杨桃。
   - `SEED_GATLINGPEA` 由"只需要双发射手"扩成"需要双发射手**或**究极电能杨桃"，
     否则互换方向会因为没有双发射手而被卡面判灰、根本拿不起来。
   （`SEED_STARFRUIT` 本来没有前提，正向互换天然可拿。）

## 改动清单

| 文件 | 改动 |
| --- | --- |
| `src/ConstEnums.h` | `SEED_ELECTRIC_STARFRUIT`、`REANIM_ELECTRIC_STARFRUIT`、`PROJECTILE_ELECTRIC_STAR = 15`（`NUM_PROJECTILES` 15→16）、`ADVICE_PLANT_NEEDS_STARFRUIT` |
| `src/GameConstants.h` | `ELECTRIC_STAR_HIT_DAMAGE` / `ELECTRIC_STAR_LINGER_TICKS` / `ELECTRIC_STARFRUIT_USE_CUSTOM_ART` / `ELECTRIC_STARFRUIT_SWITCH_REFUND` |
| `src/Sexy.TodLib/Reanimator.cpp` | 注册 `REANIM_ELECTRIC_STARFRUIT` → `reanim/Starfruit.reanim` + `REANIM_NO_ATLAS` |
| `src/Lawn/Projectile.h/.cpp` | `gProjectileDefinition` 新增 `{ PROJECTILE_ELECTRIC_STAR, 0, 30 }`；`mLingerCountdown`；`StartElectricStarLinger()` / `UpdateElectricStarLinger()`；`MOTION_STAR` 追踪与命中判定并入该类型；`Draw` 用白色滤镜剪影 + 电能蓝上色；`DrawShadow` 不画影子 |
| `src/Lawn/Plant.h/.cpp` | `gPlantDefs` 新增行（300 阳光 / 3000 冷却 / 300 生命 / 100 发射节奏 / `"ELECTRIC_STARFRUIT"`）；`ElectricStarfruitHasCustomArt()`（逐帧换三张图，带安全阀）+ `ElectricStarfruitReanimType()`；`PlantInitialize` / `PreloadPlantResources` 在建动画前解析 reanim 类型；`UpdateReanimColor` 仅在无专用贴图时整株电能蓝叠加；`UpdateShooter`/`Fire`/`ElectricStarFruitFire` 并入；`IsUpgradableTo`（杨桃→究极电能杨桃 + 两条互换）；`IsUpgrade`/`IsRedCard`；`PlantDrawHeightOffset`/`DrawShadow` 并入杨桃分支；`AttachBlinkAnim` 眨眼 reanim 类型 |
| `src/Lawn/System/ReanimationLawn.cpp` | `DrawReanimatorFrame` 先换贴图（卡面/图鉴/光标预览），并在无专用贴图时兜底染色；`MakeCachedPlantFrame` 解析 reanim 类型 |
| `src/Lawn/Travel.cpp` | `gTravelPlantDefs` 登记 `{ SEED_ELECTRIC_STARFRUIT, true }`（升级卡） |
| `src/LawnApp.cpp` | `HasSeedType`：`SEED_ELECTRIC_STARFRUIT → IsTravelLevel(mGameMode)` |
| `src/Lawn/Widget/SeedChooserScreen.cpp` | `CheckSeedUpgrade(SEED_ELECTRIC_STARFRUIT, SEED_STARFRUIT)` |
| `src/Lawn/Board.cpp` | `PlantingRequirementsMet`（新增杨桃前提；机枪射手放宽为"双发射手或究极电能杨桃"）；`MouseDownWithPlant` 互换改写 + 返还 225；提示字幕 `[ADVICE_ONLY_ON_STARFRUIT]`；种卡悬浮提示 `[REQUIRES_STARFRUIT]`；冰冻沙盒"旅行页"加入该卡 |
| `src/Lawn/SeedPacket.cpp` | `SetPacketType` 冷却豁免（保持 30.01 s）；`DrawSeedPacket` 卡面缩放/偏移并入杨桃档；种植前提提示 `[ADVICE_PLANT_NEEDS_STARFRUIT]` |
| `properties/pvzp-strings.xml`、`properties/pvzp-strings.zh-CN.xml`、`dist/pvz-portable/properties/*`、运行目录 `properties/*` | `[ELECTRIC_STARFRUIT]` / `[ELECTRIC_STARFRUIT_TOOLTIP]` / `[REQUIRES_STARFRUIT]` / `[ADVICE_PLANT_NEEDS_STARFRUIT]` / `[ADVICE_ONLY_ON_STARFRUIT]` / `[ULTIMATE_SWITCH_REFUND]` |
| `scripts/check-electric-starfruit.ps1` | 源级检查（对齐既有 `check-*.ps1` 惯例） |
| `CONTEXT.md` | 旅行子系统词条更新（究极形态互换） |

## 行为细节

### 弹丸状态机（`PROJECTILE_ELECTRIC_STAR`）

```
发射（StarFruitFire：5 颗，MOTION_STAR，速度 3.33）
  ↓ 米字方向飞行
age >= 24：锁定最靠左的可伤害僵尸（复用杨桃追踪），每帧把速度重定向到目标中心
  ↓ 与目标矩形重叠（CheckForCollision）
命中 → StartElectricStarLinger(theZombie)
  ↓ 首次命中起算总寿命 mLingerCountdown = 500（5 s @100fps）
钉住状态（UpdateElectricStarLinger，每帧）：
    - 位置跟随该僵尸中心（僵尸走动时星星跟着走）
    - 对该僵尸 TakeDamage(30, GetDamageFlags()) —— 与电能豌豆同一口径
    - 僵尸死亡 / 不再可被伤害（EffectedByDamage 为假，含濒死阶段）→ RetargetElectricStar()
        · 找到下一个目标：退出钉住状态，交还给 MOTION_STAR 追踪飞行（不改写寿命）
        · 没有目标：留在原地，等总寿命走完
    - 钉住结算完这一帧后 mLingerCountdown 减 1；归零 → Die()
改追飞行（回到 Update 的未钉住分支）：
    - 每帧照常扣总寿命（所以"5 秒"是这颗星星的**总寿命**，不是每个目标各 5 秒）
    - 命中下一个目标 → 再次 StartElectricStarLinger（不重新计时）
```

- **总寿命口径**：`mLingerCountdown` 只在**第一次命中**时置为 `ELECTRIC_STAR_LINGER_TICKS`（500），
  之后钉住与改追飞行都照常扣；归零即 `Die()`。所以一颗星星的伤害窗口恒为 5 秒，
  期间会一个接一个地追杀（普通僵尸约 9 刻即死，因此 5 秒内能连续清掉相当多目标）。
  想把"每个新目标各自重新算 5 秒"（= 只要还有僵尸就永不消失），只需把
  `StartElectricStarLinger` 里那句 `if (mLingerCountdown <= 0)` 的守卫去掉。
- **目标判定沿用引擎口径**：`Zombie::EffectedByDamage()` 本身就会拒绝**死亡/濒死**僵尸
  （除非伤害标志带 `DAMAGES_DYING`，本星星不带），因此"僵尸进入死亡动画"的那一刻
  就会被判为"目标已消失"并立刻改追下一个，不会对着尸体再打几帧。
- **改追索敌规则与杨桃一致**：取**最靠左**的可伤害僵尸（`mX` 最小），且不受行限制。
- **没追上目标**：星星继续按原方向飞；若中途撞到别的僵尸（`FindCollisionTarget`）也照样钉住；
  飞出屏幕（X 方向出界，或 Y 方向离开草坪）或总寿命耗尽则 `Die()`。
- **钉住期间不再移动**：进入钉住状态后 `Projectile::Update` 直接走 `UpdateElectricStarLinger()`，
  不调 `UpdateMotion()`（因此也不会再触发追踪/碰撞/高地判定）。
- 伤害标志沿用 `GetDamageFlags()`：`MOTION_STAR` 且 `mTargetZombieID != NULL` → **穿盾**，
  与普通杨桃星星一致。
- 绘制：`IMAGE_PROJECTILE_STAR` 过 `FILTER_EFFECT_WHITE` 取剪影后按
  `ELECTRIC_BLUE_R/G/B` 上色（与电能豌豆同一套代码路径、同一组颜色常量）→ 干净的电光蓝星星。
- 不画地面影子（`DrawShadow` 直接 return）：钉在僵尸身上时影子无意义。

### 存档兼容

- `SeedType` / `ReanimationType` / `ProjectileType` 三处枚举都是**前插**，既有值不变。
- `Projectile` 新增的 `mLingerCountdown` **追加在 `SyncProjectileTailPortable` 末尾**：
  该 tail 是逐字段顺序读写，旧存档读到末尾抛 `DataReaderException` 后把剩余字段置 0
  （`PortableSaveContext` 既有行为），新字段默认 0 = "不在钉住状态"，旧存档可正常读回。
- 弹丸是瞬时对象，跨关不保留；互换只是"改种子 + 加阳光"，不引入新存档字段。
- 不新增 `Plant` 数据成员 → `SyncPlantTailPortable` 无需改动。

## 手动验证清单（无自动化测试框架）

1. 编译通过：`cmake --build build`。
2. 源级检查：`pwsh -File scripts/check-electric-starfruit.ps1`。
3. 获得入口：
   - 冰冻关卡沙盒 → **旅行页**：出现"究极电能杨桃"红卡，卡面为电能版贴图（不再是被染黄的杨桃）。
   - 旅行：无尽旅程（自由选卡）→ **选卡器页 1**：出现该卡；选中它会连带要求页 0 的杨桃。
4. 升级种植：场上无杨桃时选卡提示"需要先种下杨桃"且无法开始；有杨桃时拖卡到杨桃格 →
   花费 **300 阳光**、冷却约 **30 秒**，该格变成究极电能杨桃。
5. 战斗：
   - 每轮 **5 颗电光蓝星星**呈米字飞出，飞行约 1 格后**拐向最近的僵尸**；
   - 命中瞬间星星**停在僵尸身上**并持续掉血（30/游戏刻），僵尸走出格子时星星跟着走；
   - **僵尸一死，星星立刻改追下一个目标**（飞过去再钉住，继续 30/游戏刻）；
     场上没有别的僵尸时它留在原地，等总寿命（首次命中起 5 秒）走完消失。
   - 5 颗全中同一僵尸时掉血明显叠加；因为普通僵尸约 9 刻即死，一颗星星在 5 秒内能连续清掉多波目标。
6. 究极形态互换：
   - 场上先有究极电能机枪射手 → 拖**杨桃**卡（125）上去 → 变成究极电能杨桃并**返还 225 阳光**；
   - 场上先有究极电能杨桃 → 拖**机枪射手**卡（250）上去 → 变回究极电能机枪射手并**返还 225 阳光**；
   - 没有究极形态时，两张基础卡仍然是各自原本的升级用途（机枪射手 → 双发射手等）。
7. 观感：究极电能杨桃的**主体星形 + 眼睛为电光蓝白**，**叶/茎保持绿色**；卡面/图鉴/光标预览一致。
8. 回归：普通杨桃的一切行为与贴图不变；究极电能机枪射手的攻击/开大不变；
   存档进出一次关卡（`.v4`）读回正常。
