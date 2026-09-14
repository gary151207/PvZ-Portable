# 设计：究极电能机枪射手（Electric Gatling Pea）— 旅行专属红卡升级植物

日期：2026-09-12（配色于 2026-09-13 由暖白改为电能蓝）
状态：已批准（2026-09-12 与用户确认设计；配色后续按需求调整为蓝）

用户原始需求：

> 添加新植物电能机枪射手为红卡植物需要种植在机枪射手进行升级花费200阳光，它的效果发射电能豌豆并且
> 其余机制跟机枪射手一样开大机制也跟机枪射手一样，贴图为机枪射手的主体部分变白枪管也变白底下和头盔不变

后续需求：

> 效果没问题就是贴图感觉还是不够白不够像电能的颜色参考电能豌豆的样子稍微黄一点
> 把电能机枪射手和电能豌豆的黄改成蓝
> 电能豌豆的蓝色似乎不太对，应该跟电能机枪射手的蓝色一样的颜色

> 备注（2026-09-13）：以上引文为用户原话，其中的旧名「电能机枪射手」在后续需求中已正式更名为
> **究极电能机枪射手**；本文档其余部分统一使用新名。C++ 内部标识符（`SEED_ELECTRIC_GATLING_PEA`、
> `REANIM_ELECTRIC_GATLINGPEA`、`ELECTRIC_GATLING_*`、`ELECTRIC_BLUE_*`）与字符串 key
> （`ELECTRIC_GATLING_PEA`）保持不变，以免破坏存档与已打包资源。

设计澄清：

| 议题 | 结论 |
| --- | --- |
| 获取范围 | **仅旅行模式**（页 1 + 传送带），另外登记进冰冻关卡沙盒的**旅行页**供直接测试 |
| 子弹组成 | **100% 电能豌豆**（不再发射普通绿豌豆） |
| 电能豌豆是什么 | **就是现有的 `PROJECTILE_FIREPEA_RED`**（原"红火豌豆"，旧实现是纯白闪电豌豆贴图），不新增弹丸类型 |
| 电能豌豆颜色 | **电能蓝**，与植物**共用同一组颜色常量** `ELECTRIC_BLUE_R/G/B`（默认 200,240,255） |
| 贴图 | **优先用玩家自备的专用贴图**（`reanim/ElectricGatling_*`，见下）；贴图缺失或不可用时回退到"机枪射手贴图 + 电能蓝叠加" |
| 开大机制 | 与机枪射手**完全一致**（散射大招，触发/时长/扇形/概率成长逻辑照抄） |
| 头盔与底下 | **保持不变**（头盔轨道双重豁免；底部叶/茎所在的 body 实例完全不着色） |

## 背景与目标

旅行模式子系统（`src/Lawn/Travel.h/.cpp`）已落地，旅行专属植物现有三只：
`SEED_FUMESHROOM_GROUP`（紫卡升级卡）、`SEED_GIANT_WALLNUT`（红卡）、`SEED_PEATER_1_5`（红卡）。
本次新增第四只，也是第一只**同时是升级卡（紫卡语义）又是红卡**的植物：

1. **纯新增枚举** `SeedType::SEED_ELECTRIC_GATLING_PEA`，前插在 `SEED_PEATER_1_5` 之后、
   `NUM_SEED_TYPES` 之前，**不动任何既有枚举值** → 存档稳定。
2. **升级关系**：种植前提 = 场上有**机枪射手**（`SEED_GATLINGPEA`），拖到它身上原地升级
   （走引擎既有升级卡路径 `Board::MouseDownWithPlant`）。
3. **数值**：200 阳光、30.01 s 冷却（沿用机枪射手的 `mRefreshTime = 3000`）、300 生命、
   `mLaunchRate = 100`（与机枪射手同节奏）、普通射手攻击节奏。
4. **卡面**：红卡（`Plant::IsRedCard`）+ 升级卡（`Plant::IsUpgrade`）——卡包底色走红卡着色渲染。
5. **仅旅行模式**：登记进 `gTravelPlantDefs`（→ `IsTravelOnlySeed` → 选卡器页 1），
   `LawnApp::HasSeedType` 仅 `IsTravelLevel(mGameMode)` 返回真。
6. **子弹**：100% 发射 `PROJECTILE_FIREPEA_RED`（电能蓝闪电豌豆：接触每 tick 30 点伤害、
   无限穿透、出屏才消失）。
7. **开大（散射大招）**：完全照抄机枪射手的 `mGatlingScatterCountdown` / `mGatlingScatterChance` 机制。
8. **贴图**：复用 `REANIM_GATLINGPEA`；**头部实例**（头/脸/嘴/眉毛/眨眼/枪管）整体染成电能蓝，
   `GatlingPea_helmet` 轨道豁免；**body 实例**（叶/茎 = "底下"）原样不动。

**非目标**：
- 不新增弹丸类型（复用 `PROJECTILE_FIREPEA_RED`）。
- 不新增美术 / 新 Reanimation（复用 `reanim/GatlingPea.reanim`）。
- 不改机枪射手本体（`SEED_GATLINGPEA`）行为，不影响魅惑机枪头僵尸（`ZOMBIE_GATLING_HEAD`，它复用同一
  reanim 但走 `Zombie.cpp` 独立路径，本次不碰）。
- 不做自动化测试（仓库无测试框架），以编译 + 源级检查 + 手动试玩验证。

## 现状调研

### 机枪射手的三个"机制点"

| 机制 | 位置 | 行为 |
| --- | --- | --- |
| 每轮 4 连发 | `Plant::UpdateShooting`（Plant.cpp ~L3768） | `mShootingCounter == 18/35/51/68` 各 `Fire()` 一次 |
| 开大（散射大招） | `Plant::FindTargetAndFire`（~L842）、`Plant::UpdateShooting`（~L3770）、`Plant::Fire`（~L5302） | 每轮开始 `Rand(100) < mGatlingScatterChance` → `mGatlingScatterCountdown = 300`（3 s）；散射期间每 2 帧发射 2 颗 ±10° 扇形子弹（`MOTION_STAR`）；正常每发主弹 50% 概率 `mGatlingScatterChance++`（上限 100，散射结束重置为 3） |
| 3% 特殊弹 | `Plant::Fire`（~L5198 `RollGatlingBulletType`） | 每颗子弹独立 `Rand(100) < 3` → `PROJECTILE_FIREPEA_RED` |

### "红火豌豆"就是电能豌豆

`Projectile.cpp`：

- 定义表：`{ ProjectileType::PROJECTILE_FIREPEA_RED, 0, 30 }`（`mDamage = 30`）。
- `CheckForCollision`（~L443）：**电能伤害**——每 tick 对矩形重叠的每个僵尸造成 30 点伤害；
  **无限穿透**，永不因击中而消亡，只在出屏 / 高地过渡时 `Die()`。
- `Draw`（~L1282）：旧实现是 `IMAGE_PROJECTILEPEA` 用**加色叠加 4 次**把通道饱和到 255 → "纯白"豌豆。

即：`PROJECTILE_FIREPEA_RED` 与"电能豌豆"是同一个东西，因此本特性**不需要新增弹丸类型**，
只需让究极电能机枪射手 100% 发射它，并把它的颜色改成电能蓝。

### GatlingPea.reanim 的轨道结构（决定"哪些着色"）

从 `dist/pvz-portable/main.pak` 解出 `reanim/GatlingPea.reanim`（22 条轨道，fps=12，93 帧时间轴）：

| 区间 | 轨道 | 对应部位 | 本次处理 |
| --- | --- | --- | --- |
| `anim_idle`（帧 4-28） | `backleaf`、`backleaf_left_tip`、`backleaf_right_tip`、`stalk_bottom`、`stalk_top`、`frontleaf`、`frontleaf_right_tip`、`frontleaf_tip_left` | **底下**（叶 / 茎；复用 `PEASHOOTER_*` 贴图） | **不变** |
| `anim_head_idle` / `anim_shooting`（帧 29-92） | `anim_face`（=`GATLINGPEA_HEAD`）、`GatlingPea_mouth`、`GatlingPea_mouth_overlay`、`PeaShooter_eyebrow`、`anim_blink` / `idle_shoot_blink`（=`GATLINGPEA_BLINK*`） | **主体**（头 / 脸 / 五官） | **染成电能蓝** |
| 同上 | `GatlingPea_barrel1`~`barrel4`（共用 `GATLINGPEA_BARREL` 贴图） | **枪管** | **染成电能蓝** |
| 同上 | `GatlingPea_helmet` | **头盔** | **不变** |

**关键机制**：`Plant::PlantInitialize`（Plant.cpp ~L235-275）对射手创建**两个实例、同一份 reanim**：

- **body 实例** `mBodyReanimID`：播 `anim_idle` → 只显示叶/茎（"底下"）；
- **head 实例** `mHeadReanimID`：播 `anim_head_idle`，并
  `AttachToAnotherReanimation(aBodyReanim, "anim_idle")`（GatlingPea 无 `anim_stem`）。

因此"主体 + 枪管着色、底下不变"天然就是**"只处理 head 实例"**。

### 专用贴图（首选路径）

玩家把六张专用部位贴图打包进了 `main.pak`：

| 文件（`reanim/` 下） | 用途 |
| --- | --- |
| `ElectricGatling_head.png` | 头/脸（`anim_face` 轨道） |
| `ElectricGatling_mouth.png` | 嘴 |
| `ElectricGatling_mouth_overlay.png` | 嘴（覆盖层） |
| `ElectricGatling_barrel.png` | 枪管（`GatlingPea_barrel1~4` 共用同一张） |
| `EletricGatling_blink1.png` / `EletricGatling_blink2.png` | 眨眼（**注意 pak 里的拼写是 `Eletric`，少了 c**，代码按原名照抄） |

接入方式：**新增一个 `ReanimationType::REANIM_ELECTRIC_GATLINGPEA`**，它加载与机枪射手**同一个**
`reanim/GatlingPea.reanim`，但装载后由 `ElectricGatlingHasCustomArt()` 把定义里的贴图换成上表这一套。
开关在 `src/GameConstants.h` 的 `ELECTRIC_GATLING_USE_CUSTOM_ART`（当前为 `true`）。

- `ReanimatorEnsureDefinitionLoaded()` 是**按 ReanimationType 分槽**装载的，两个类型指向同一文件
  各自得到一份独立定义，所以**改它不影响普通机枪射手**，也不影响复用同一 reanim 的魅惑机枪头僵尸
  （`ZOMBIE_GATLING_HEAD`）。
- **本类型用 `REANIM_NO_ATLAS`（不建自己的 Atlas）**。这是刻意的：有了 Atlas，`ReanimAtlasCreate()`
  会把定义里的 `mImage` **改写成"图集下标"这种假指针**，于是"换贴图"就必须**赶在建 Atlas 之前**完成，
  时机错了就静默失效；而且还要和机枪射手那份 Atlas 共用同一批源贴图。关掉 Atlas 后定义里始终是
  真实贴图指针，换图在任何时机都安全（`PreloadPlantResources` / `PlantInitialize` / `DrawReanimatorFrame`
  三处仍会调用，函数幂等）。代价是这种植物不走图集批绘制，只有它一只，可以忽略。
- **必须按"原图指针"逐帧替换，不能按轨道挂 `mImageOverride`**：`anim_blink` 与 `idle_shoot_blink`
  这两条轨道**同时引用 blink1 和 blink2 两张图**（不同帧换图），而 `mImageOverride` 是"整轨一张图"，
  会把眨眼动画压成同一张。
- **专用贴图必须自带透明通道，且尺寸与被替换的原贴图完全一致**（reanim 是按帧号索引到贴图某一列的，
  尺寸不符会串帧）。接线时用 `MemoryImage::CommitBits()` 现算 `mHasTrans` / `mHasAlpha`
  （这两个标志平时要等首帧绘制 `TodSandImageIfNeeded()` 才算），再比对两者宽高；
  **任何一张不合格就整体回退**（返回 false → 走下面的染色兜底），避免半套贴图 / 白方块 / 串帧。
  > 踩过的坑：最初打包进来的六张图**背景是白色且不透明**（每张 alpha 全是 255），
  > 程序按上面的规则整体回退，表现为"换了贴图却没变化"。抠掉白底（只删与边缘连通的纯白像素）
  > 重新打包后才生效；`docs/superpowers/specs/electric-gatling-art-fixed/` 里留了抠好底的一份。

### 兜底：电能蓝叠加（专用贴图不可用时）

只有 `ElectricGatlingHasCustomArt()` 返回 false 时才走这条老路：把 head 实例整体染成电能蓝，
头盔保持原色。**不能**用"额外加色绘制"（`mExtraAdditiveColor`）：加色按原图像素成比例相加，
头部贴图蓝通道只有 ~36，永远调不出蓝/白。所以走 **`mEnableExtraOverlayDraw`**
（把轨道贴图换成白色滤镜版再混上来），并给 `ReanimatorTrackInstance` **新增 `mIgnoreExtraOverlayColor`**
（与既有 `mIgnoreExtraAdditiveColor` 对称）把头盔排除在外。

### 为什么兜底染色必须用"白色滤镜叠加绘制（extra overlay draw）"

引擎的着色通道（`Reanimator.cpp` `Reanimation::DrawTrack`）：

| 通道 | 作用域 | 能否按轨道豁免 | 混合公式 | 效果 |
| --- | --- | --- | --- | --- |
| `mColorOverride` / `mTrackColor` | 逐轨道 | 是 | `ColorsMultiply` 乘色 | **只能变暗** |
| `mEnableExtraAdditiveDraw` | 整个 reanim | 是（`mIgnoreExtraAdditiveColor`） | `dst += 原图像素 × 颜色 × A/255` | **按原图像素成比例相加 → 调不出蓝/白** |
| `mEnableExtraOverlayDraw` | 整个 reanim | 原为**否** | `dst = 颜色 × A/255 + 原色 × (1-A/255)` | 贴图先过白色滤镜（RGB=255、alpha 不变）再混上来 → **颜色可控** |

> **踩过的坑（早期两版都错在这）**：加色是**按原图像素成比例**相加的，不是加一个常量。
> `GatlingPea_head.png` 的均值是 (128,169,36)、**蓝通道只有 36**，加色最多把 R/G 顶到 255，
> 蓝通道怎么加都上不去 → 结果始终是**偏黄绿**，无论 alpha 调多大都不够白/不显蓝。
> 首版用纯白 `alpha=150`、第二版用 `255,245,190@245`，两版都是这个原因。
> 同样的坑也发生在电能豌豆上：4 次白色加色虽然把主体乘到饱和，但豌豆贴图**接近纯黑的描边像素
> 乘 4 次仍然很暗**，所以"纯白豌豆"实际带着一圈没被盖掉的绿黑描边。

**结论：用 `mExtraOverlayColor = Color(ELECTRIC_BLUE_R, ELECTRIC_BLUE_G, ELECTRIC_BLUE_B, ELECTRIC_GATLING_TINT_A)`
+ `mEnableExtraOverlayDraw = true` 施加在 head 实例上**；由于 overlay 原本没有逐轨道豁免，
本次给 `ReanimatorTrackInstance` **新增 `mIgnoreExtraOverlayColor`**（与既有
`mIgnoreExtraAdditiveColor` 对称），在 `DrawTrack` 的 atlas 与非 atlas 两条路径上都生效，
用它把 `GatlingPea_helmet` 排除在外。

### 颜色取值（兜底染色与电能豌豆共用同一组颜色）

`src/GameConstants.h`：

```cpp
constexpr const int ELECTRIC_BLUE_R = 200;
constexpr const int ELECTRIC_BLUE_G = 240;
constexpr const int ELECTRIC_BLUE_B = 255;
constexpr const int ELECTRIC_GATLING_TINT_A = 245;   // 只影响植物的叠加强度
```

- **植物**（overlay）：最终色 = 颜色 × A/255 + 原色 × (1-A/255)。A=245 时约等于颜色本身，
  只掺进 4% 原色以保留极淡的五官与描边。用 `reanim/GatlingPea_*.png` 真实像素标定：

  | 精灵 | 原图均值 | 旧版加色（错） | **overlay 200,240,255@245** |
  | --- | --- | --- | --- |
  | `head`（主体） | (128,169,36) 黄绿 | 偏黄绿、蓝色上不去 ✗ | **(197,237,247) 干净的电光蓝** |
  | `mouth` | (84,115,36) | 同上 | (199,236,250) |
  | `barrel`（枪管） | (128,130,123) 灰 | 发灰 | (198,237,251) |

- **豌豆**（白色滤镜剪影 + 正常绘制，alpha 255）：最终色 = 颜色 = (200,240,255)。
  与植物实测 (197,237,247) 的差 ≤ 8（4% 掺色造成），肉眼一致。
- 两个渲染路径**共用 `ELECTRIC_BLUE_R/G/B`**，所以调色只需改一处，二者不会各自跑偏。
- 调色方向：整体越接近 255,255,255 越白；压低 R/G 越蓝。

对比参考图：`docs/superpowers/specs/2026-09-12-electric-gatling-pea-tint.png`。

### 电能豌豆的上色（`Projectile.cpp`）

`Projectile::Draw` 的 `PROJECTILE_FIREPEA_RED` 分支：

```cpp
Image* aSilhouette = FilterEffectGetImage(aImage, FilterEffect::FILTER_EFFECT_WHITE);
TodBltMatrix(g, aSilhouette, aTransform, g->mClipRect,
             Color(ELECTRIC_BLUE_R, ELECTRIC_BLUE_G, ELECTRIC_BLUE_B, 255), g->mDrawMode, aSrcRect);
```

`FilterEffectGetImage(..., FILTER_EFFECT_WHITE)` 走 `FilterEffectDoWhite`（`*ptr |= 0x00FFFFFF`）：
**RGB 强制 255、alpha 不变**，于是"剪影 + 上色"得到**纯色干净的电能蓝豌豆**，原本的绿黑描边
也一起被替换掉（旧版 4 次加色盖不掉它）。需要 `#include "../Sexy.TodLib/FilterEffect.h"`。

> `REANIM_GATLINGPEA` 的 `ReanimFlags` 为 0 → atlas 打包（`Reanimator.cpp` ~L121），走 atlas 路径。

### 让 head 实例的着色不被覆盖

head 是 body 的 attachment，其颜色状态由 `Plant::UpdateReanimColor`（Plant.cpp ~L3005-3120）
末尾的 `aBodyReanim->PropogateColorToAttachments()` 每帧**从 body 传播覆盖**
（`Attachment.cpp` ~L313-317 会连 `mExtraOverlayColor` / `mEnableExtraOverlayDraw` 一起写过去）。

因此本特性的着色必须在 `PropogateColorToAttachments()` **之后**设置（`UpdateReanimColor` 里），
并且：

- **Beghouled 闪白**（body 走 overlay 通道并传播到 head）：让位给闪光，不叠加本配色；
- **手持升级卡高亮 / 被啃食**（走加色通道）：head 照常叠加本配色，闪光叠在上面，视觉正常。

头盔的两条豁免标志（`mIgnoreExtraAdditiveColor` + 新增的 `mIgnoreExtraOverlayColor`）
**逐帧在 `UpdateReanimColor` 里设置**，而不是只在 `PlantInitialize` 设一次——因为新标志
**不写入存档**（避免动 `.v4` 里 `SyncReanimTrackInstancePortable` 的字段布局破坏旧存档），
逐帧设置可以让**读档后也立刻恢复**（植物是从存档重建的，不会重跑 `PlantInitialize`）。

body 实例本身**不做任何着色** → 叶/茎（底下）保持原色。

## 改动清单

| 文件 | 改动 |
| --- | --- |
| `src/ConstEnums.h` | `SEED_ELECTRIC_GATLING_PEA`（插在 `SEED_PEATER_1_5` 之后、`NUM_SEED_TYPES` 之前）；`REANIM_ELECTRIC_GATLINGPEA`（插在 `REANIM_FLAG` 之后、`NUM_REANIMS` 之前）；`ADVICE_PLANT_NEEDS_GATLINGPEA` |
| `src/GameConstants.h` | `ELECTRIC_BLUE_R/G/B`（兜底染色与电能豌豆共用的电能蓝）+ `ELECTRIC_GATLING_TINT_A`（叠加强度），唯一观感调参点 |
| `src/Lawn/Plant.cpp` | `gPlantDefs` 改用 `REANIM_ELECTRIC_GATLINGPEA`；新增 `ElectricGatlingHasCustomArt()`（装载并逐帧替换专用贴图，带透明通道安全阀）；`PlantInitialize` / `PreloadPlantResources` 在建动画前调用它；`UpdateReanimColor` 的染色改为仅在无专用贴图时兜底；`FindTargetAndFire` / `UpdateShooting` / `Fire`（弹丸类型、抛射音效、枪口坐标、散射大招、概率成长、孤狼伤害）各机枪分支并入；`IsUpgradableTo` 机枪射手→究极电能机枪射手；`IsUpgrade`、`IsRedCard`、`AttachBlinkAnim` 眨眼列表 |
| `src/Lawn/Projectile.cpp` | `Projectile::Draw` 的 `PROJECTILE_FIREPEA_RED` 分支改为"白色滤镜剪影 + 按 `ELECTRIC_BLUE_*` 上色"；include `FilterEffect.h` |
| `src/Sexy.TodLib/Reanimator.h` | 新增 `ReanimationType::REANIM_ELECTRIC_GATLINGPEA` 的枚举；`ReanimatorTrackInstance` 新增 `mIgnoreExtraOverlayColor`（逐轨道豁免"白色滤镜叠加绘制"，与既有 `mIgnoreExtraAdditiveColor` 对称） |
| `src/Sexy.TodLib/Reanimator.cpp` | 注册 `REANIM_ELECTRIC_GATLINGPEA` → `reanim/GatlingPea.reanim`（与机枪射手同文件、独立定义槽）；构造函数初始化新标志；`DrawTrack` 的 **atlas 与非 atlas 两条 overlay 路径**都检查该标志 |
| `src/Lawn/System/ReanimationLawn.cpp` | `DrawReanimatorFrame` 先装载专用贴图再 `ReanimationInitializeType`（Atlas 在那里建）；head 层的电能蓝染色同样改为"无专用贴图才兜底"；`MakeCachedPlantFrame` 头部绘制列表并入该种子 |
| `src/Lawn/Travel.cpp` | `gTravelPlantDefs` 登记 `{ SEED_ELECTRIC_GATLING_PEA, true }`（升级卡） |
| `src/LawnApp.cpp` | `HasSeedType` 新增 `case SEED_ELECTRIC_GATLING_PEA: return IsTravelLevel(mGameMode);` |
| `src/Lawn/Widget/SeedChooserScreen.cpp` | `CheckSeedUpgrade(SEED_ELECTRIC_GATLING_PEA, SEED_GATLINGPEA)` |
| `src/Lawn/Board.cpp` | `PlantingRequirementsMet` 要求场上有机枪射手；种卡提示 `[REQUIRES_GATLINGPEA]`；HP 悬浮提示的散射显示并入；冰冻沙盒"旅行页"加入该卡 |
| `src/Lawn/Challenge.cpp` | TRAVEL_2 传送带阵容加入该卡（权重 10）。**旅行关都是传送带关**（`gTravelLevelDefs` 的 `mConveyorBelt = true`），选卡器页 1 在真实旅程里不会出现，所以传送带是实际唯一的获得途径；它升级自机枪射手，故与机枪射手同放 TRAVEL_2 |
| `src/Lawn/SeedPacket.cpp` | `SetPacketType` 冷却豁免并入（保持 30.01 s）；`DrawSeedPacket` 卡面缩放/偏移并入；种植前提提示 `[ADVICE_PLANT_NEEDS_GATLINGPEA]` |
| `properties/pvzp-strings.xml`、`properties/pvzp-strings.zh-CN.xml`、`dist/pvz-portable/properties/*` | `[ELECTRIC_GATLING_PEA]` / `[ELECTRIC_GATLING_PEA_TOOLTIP]` / `[REQUIRES_GATLINGPEA]` / `[ADVICE_PLANT_NEEDS_GATLINGPEA]`（中英） |
| `scripts/check-electric-gatling-pea.ps1` | 源级检查（对齐既有 `check-*.ps1` 惯例） |
| `CONTEXT.md` | 旅行子系统词条更新 |

## 行为细节

- **升级路径**：完全复用引擎升级卡路径。`Board::MouseDownWithPlant` → 扣 200 阳光 +
  走冷却 → `Plant::IsUpgradableTo(SEED_ELECTRIC_GATLING_PEA)` 成立
  （`mSeedType == SEED_GATLINGPEA`）→ 原机枪射手 `Die()` → `AddPlant(同格, 究极电能机枪射手)`。
  下层（睡莲 / 花盆 / 南瓜）不受影响；被蹦极抓取中不可升级（`CanPlantAt` 既有约束）。
- **冷却**：`mRefreshTime = 3000`。`SeedPacket::SetPacketType` 对 `IsUpgrade` 有 3500 / 8000
  的通用覆写，会把它压成 3500 → 与 `SEED_GATLINGPEA` / `SEED_GIANT_WALLNUT` 一样**加豁免**，
  保持 30.01 s 档位。
- **子弹 100% 电能**：`Fire()` 的 `switch (mSeedType)` 直接给出
  `PROJECTILE_FIREPEA_RED`。**不参与** `RollGatlingBulletType`（那是机枪射手 3% 掷骰用的，
  只对 `SEED_GATLINGPEA` 生效，保持原样不动）。
- **开大**：`mGatlingScatterChance` 初值 3；每轮**开始的判定**在 `FindTargetAndFire` 里
  （`mShootingCounter` 被设为 100 那一帧）；散射期间 `UpdateShooting` 每 2 帧 `Fire()` 一次，
  `Fire()` 内每发发 2 颗 ±10° 扇形弹（`MOTION_STAR`）。
  散射子弹同样是 `PROJECTILE_FIREPEA_RED`（穿透 + 每 tick 30 点），
  `mDamageOverride = 200` 对它不生效（灼烧走固定伤害），这与"开大机制一样"不冲突——
  机制（触发、时长、扇形、射速）一致，伤害沿用弹丸本身的定义。
- **抛射音效**：`FOLEY_THROW` 在散射模式下抑制，判据从 `SEED_GATLINGPEA` 扩为
  "机枪射手或究极电能机枪射手"。
- **孤狼关卡**：机枪射手的 `mDamageOverride = 200` 分支并入本种子（孤狼模式不产究极电能机枪射手，
  但并入可避免行为分叉）。
- **眨眼**：`AttachBlinkAnim` 的豌豆家族列表并入本种子，保证 `anim_blink` / `idle_shoot_blink`
  能正确挂到头部 attachment。

## 存档兼容

- 只**前插**枚举值，既有 `SeedType` / `StoreItem` 数值全部不变。
- 不新增 `Plant` 数据成员 → `SyncPlantTailPortable` 无需改动。
- 新增的 `mIgnoreExtraOverlayColor` **不写入 `.v4`**，改为逐帧设置，因此
  `SyncReanimTrackInstancePortable` 的字段布局不变，旧存档可正常读回。
- 弹丸为瞬时对象（不入档）；`mGatlingScatterCountdown` 等既有 mod 计时器本来就不入档。
- `.v4` 存档读回正常。

## 手动验证清单（无自动化测试框架）

1. 编译通过：`cmake --build build`。
2. 源级检查：`pwsh -File scripts/check-electric-gatling-pea.ps1`。
3. **专用贴图必须是带透明通道的 PNG**：用它自己的背景色不透明时，程序会**整体回退**到
   "机枪射手贴图 + 电能蓝叠加"（宁可不好看也不画白方块），表现为"换了贴图却没变化"。
   校验方法：`reanim/ElectricGatling_*.png` 里要有 alpha=0 的像素；本项目已按此把
   `docs/superpowers/specs/electric-gatling-art-fixed/` 里那六张（白底已抠掉）产出，可直接回填 pak。
4. 进入旅行模式可获得它的入口：
   - **巨大坚果体验关（TRAVEL_2）传送带**：应能刷出"究极电能机枪射手"红卡（与机枪射手同场，先种机枪射手再升级）。
   - 冰冻关卡沙盒 → **旅行页**：应出现该卡，卡面为红卡底色、用的是专用贴图（不再是染色版）。
   - 旅行关选卡器页 1（目前所有旅行关都是传送带关，该页仅在将来的"自由选卡旅行关"才会出现）。
4. 种植前提：场上没有机枪射手时选卡会提示需要机枪射手且无法开始；场上有机枪射手时，
   拖卡到机枪射手格 → 花费 **200 阳光**、该格变成究极电能机枪射手（机枪射手消失）。
5. 战斗：
   - 每轮 **4 发**，节奏与机枪射手一致；**每一发都是电能蓝豌豆**（纯色、无残留绿边，
     与植物同一色调）；
   - 电能豌豆命中僵尸时**持续掉血并穿透**，穿过整排后从屏幕右侧飞出消失；
   - **开大**：连续作战若干轮后应有散射状态（每 2 帧 1 次、每次 2 颗 ±10°），持续约 3 秒；
     机枪射手的概率成长（每发 50% +1%，上限 100%，结束后重置为 3%）同样生效。
6. 观感：种下的究极电能机枪射手**主体（头/脸/五官）与枪管明显呈电能蓝**，
   **底部叶/茎保持绿色**，**头盔保持原色**；会正常眨眼、开枪、可被铲掉。
7. 回归：普通机枪射手的 3% 特殊弹、4 连发、开大机制全部不变；
   魅惑机枪头僵尸（`ZOMBIE_GATLING_HEAD`）不会被染色。
8. 存档：进出一次关卡存档（`.v4`）读回正常（本次只前插枚举，未改动既有值）。
