# 旅行模式专属红卡植物：巨大坚果（Giant Wall-nut）— 设计文档

日期：2026-09-06
状态：已确认（用户答复：机制 = "双坚果底座 + 占两格"；红卡 = 红色卡面 + 旅行专属；数值 = 200 阳光 / 50 秒冷却 / 32000 生命 / 阻挡跳跃）

## 背景与目标

旅行模式子系统已落地（`src/Lawn/Travel.h/.cpp` 数据表 + 选卡器翻页 + 传送带 + 旅行专属植物
`SEED_FUMESHROOM_GROUP` 紫卡先例）。仓库现有 `SEED_GIANT_WALLNUT` 枚举与 `gPlantDefs` 行，
但目前它**只作为坚果保龄球 2（Wall-nut Bowling 2）的滚球**（`UpdateBowling` 直线滚、碰撞 1800 伤害），
没有任何"可种植的防御植物"行为。本次把它做成一张**旅行模式专属红卡植物**：

1. 复用 `SeedType::SEED_GIANT_WALLNUT`，注册为旅行专属植物（进 `TravelPlantDef` 表、
   旅行关 `HasSeedType` 特判、选卡器页 1 / 传送带可获取；普通模式不可选/不可拥有）。
2. **红卡卡面**：目前卡面只有绿卡（普通）与紫卡（`Plant::IsUpgrade` 升级卡）两类；
   为巨大坚果新增"红卡"视觉卡面（不新增美术，用既有卡包图 + 红色着色渲染）。
3. **底座机制**：红卡拖到场上**已种的一颗坚果（`SEED_WALLNUT`）格**上，且
   **同一行左或右相邻格还必须有另一颗坚果**；两颗坚果作为底座被消耗（合并），
   原地生成**一只巨大坚果，占该行连续两格**。这两格此后视为一个整体：
   不能再种其它植物、被啃/被铲/被蹦极抓取等按整体处理。
4. **数值**：阳光 200、冷却 50 秒、生命 **32000**（约为高坚果 8000 的 4 倍）。
5. **阻挡跳跃**：撑杆跳僵尸与跳跳僵尸无法跳过它（如同高坚果的拦截语义，
   撞上后原地啃食）。
6. **不影响既有用途**：坚果保龄球 2 中它仍是可滚的"巨大坚果球"（状态分支隔离）。

**非目标**：
- 不改动保龄球 2 的滚球行为（种植行为只在非保龄球状态下生效）。
- 不做任何新美术/新 Reanimation（复用 `REANIM_WALLNUT` + 现成着色/缩放机制）。
- 不改动普通模式行为；红卡、双底座、旅行专属只作用于旅行相关路径（含其旅行卡面）。
- 不做自动化测试（仓库无测试框架），以编译 + 手动试玩清单验证。

## 现状调研

### 既有 SEED_GIANT_WALLNUT 代码足迹（改动前的基线）

- `ConstEnums.h`：`SEED_GIANT_WALLNUT` 在 `NUM_SEED_TYPES` 前（与 `SEED_EXPLODE_O_NUT` 等自定义植物同区）。
- `Plant.cpp` `gPlantDefs[]`：`{ SEED_GIANT_WALLNUT, nullptr, REANIM_WALLNUT, 2, 0, 2000, SUBCLASS_NORMAL, 0, "GIANT_WALLNUT" }`
  → 当前 0 阳光、20 秒冷却（mRefreshTime=2000）；mPlantName="GIANT_WALLNUT" 决定
  `GetNameString`/`GetToolTip` 的 `[GIANT_WALLNUT]` / `[GIANT_WALLNUT_TOOLTIP]` 翻译键。
- 保龄球分支（`Plant::UpdateBowling` 等，Plant.cpp ~L2650-2800）：滚球速度 ×2、
  碰撞 1800 伤害、不弹跳改行等 —— 全部在保龄球关卡上下文生效。
- 种植形态已有骨架：`PlantInitialize` 给它 4000 血（WALLNUT 同款）；渲染 2.0 倍缩放
  （Plant.cpp ~L3073-3078，`aOffsetX -= 76; aOffsetY -= 64`）——已具备"巨大"观感，
  只是当前没有正常种植入口。
- 坚果家族通用处理已含它：`AttachBlinkAnim`/`DoBlink` 眨眼（~L3213、L3352）、
  受伤闪白等。`AnimateNuts`（裂纹脸）目前只覆盖 WALLNUT/TALLNUT —— 需扩展。

### 旅行模式既有接线（紫卡先例 = 本设计的模板）

- `Travel.h/.cpp`：`TravelPlantDef { mSeedType; bool mUpgradeCard; }`、
  `gTravelPlantDefs[]`、`IsTravelOnlySeed()`；`LawnApp::HasSeedType` 对旅行专属
  返回 `IsTravelLevel(mGameMode)`（LawnApp.cpp ~L2692）。
- 选卡器页 1 由 `gTravelPlantDefs` 驱动（SeedChooserScreen.cpp：页 1 网格只放旅行专属种子）；
  旅行体验关当前为传送带关，传送带种子表在 Challenge.cpp（~L1853，`IsTravelLevel` 分支，
  现含 小喷菇/大喷菇/大喷菇群/魅惑菇/荷叶/海蘑菇）。
- 传送带/卡槽卡片绘制走 `DrawSeedPacket(DrawSeedPacketType)`（SeedPacket.cpp）：
  卡包底色 `aPacketBackground` 按种子类型选 `IMAGE_SEEDS` 图集列（0..8，
  `Plant::IsUpgrade` → cel 1 紫卡，其余默认 2 绿卡）。红卡需在底色绘制上加红色着色。

### 两格植物与"升级替换"先例

- 玉米加农炮（SEED_COBCANNON，Kernel-pult 升级而来）是现成"占两格"先例：
  `Board::IsValidCobCannonSpot` + 种植/铲除对相邻格的占用处理 —— 两格占位的
  "占用右格"与"整体铲除"机制照它实现。
- 紫卡升级种植：`Plant::IsUpgrade` + `Plant::IsUpgradableTo` 决定 `CanPlantAt` 返回
  `PLANTING_OK`（种在已种原植物上）→ 种植路径先 `Die()` 旧植物再 `AddPlant` 新形态
  （Board.cpp 种植函数 ~L4405-4412）—— 巨大坚果的"双底座替换"在此路径上扩展。

### 跳跃僵尸（撑杆跳/跳跳）与高坚果拦截

- （实现计划 Task 落点以代码侦察结论为准；语义对齐高坚果：跳跃类僵尸遇巨大坚果
  不得起跳、改为原地啃食。）

## 设计

### 一、红卡卡面（新增"卡级"概念，最小侵入）

- 新增 `Plant::IsRedCard(SeedType)` 静态判定（对标 `IsUpgrade` 列表式实现，
  紫卡观感本就是全局列表决定而非旅行表字段——`TravelPlantDef.mUpgradeCard` 无人读取），
  目前仅 `SEED_GIANT_WALLNUT` 为红卡；`SeedPacket.cpp` 底色绘制分支：
  紫卡（cel 1）→ 红卡（绿色 cel 2 + **红色着色**，图集无红 cel）→ 普通（cel 2 原色）。
  着色只作用于卡包底色绘制段，卡片小图/阳光数字保持原样（Graphics 子画布
  SetColor/SetColorizeImages 局部开关，参照 `theGrayness/thePercentDark` 的既有做法）。
- 红卡在选卡器页 1、卡槽、传送带一致生效；普通模式的保龄球 2 卡面也随之显示红卡
  （同一物种的同一张卡，观感统一、实现一致）。
- 冷却 50s 的数值 5000 会被 `SeedPacket::SetPacketType` 的通用"5000 或升级卡"分支压成
  3500 + 预冷却 → 照 GATLINGPEA 给 GIANT_WALLNUT 加例外，保持 50s 档位。

### 二、旅行专属注册与可获取性

- `gTravelPlantDefs` 增加 `{ SEED_GIANT_WALLNUT, /*mUpgradeCard*/false }`
  （`TravelPlantDef` 结构不变；红卡观感由 `Plant::IsRedCard` 决定，与紫卡同构）。
- `LawnApp::HasSeedType`：`case SEED_GIANT_WALLNUT: return IsTravelLevel(mGameMode);`
  （`SEED_GIANT_WALLNUT=50`，当前 default 分支下它在普通模式不可拥有 ✓，旅行关可拥有需特判）。
- 传送带（当前旅行体验关）种子表**同时加入坚果与巨大坚果**——坚果是融合底座必需品，
  只给红卡不给底座坚果等于不可用（大喷菇群"既是页 1 又进传送带"的先例同样适用）。
- 选卡同选校验：巨大坚果不是紫卡升级卡，不需要"同选基础植物"；
  双底座是**场上**前提，不是选卡前提（选卡页带不带坚果都可，种的时候必须场上已有）。

### 三、双坚果底座融合 + 占两格（核心种植机制）

拖拽/落格语义（对标紫卡"种在已种植物上"，但底座是**两颗坚果**）：

1. 光标带巨大坚果卡时，可放行判定 `Board::CanPlantAt` 扩展：
   - 目标格 (x,y) 必须有 `SEED_WALLNUT`（普通坚果，非被蹦极抓取中），且
   - (x-1,y) 或 (x+1,y) 在棋盘内（0 ≤ col ≤ 8）且也有一颗 `SEED_WALLNUT`；
   - 满足则 `PLANTING_OK`，否则新增失败原因（如 `PLANTING_NEEDS_TWO_WALLNUTS`，
     走既有"无法种植"提示通道）。
2. 高亮：悬停可融合的坚果时闪白放行（复用 `UpdateReanimColor` 的
   `IsPartOfUpgradableTo` 高亮路径并扩展巨大坚果特判）。
3. 落格执行（种植函数扩展）：两颗坚果 `Die()`（带被吃/消失粒子可选），
   以**左侧格**为锚点 `AddPlant` 一只巨大坚果（`mPlantCol=min col`，同行）。
4. 占两格：
   - 巨大坚果逻辑上占据 (锚点列, row) 与 (锚点列+1, row) 两格；
   - 右格占用仿玉米加农炮：`CanPlantAt` 对"与巨大坚果重叠的格"拒绝种植，
     `GetPlantsOnLawn`/铲除/蹦极抓取等按整体找到同一只植物处理；
   - 铲子铲它 / 被吃光 → 两格整体消失（一个 Plant 对象 Die，无残留）；
   - 蹦极僵尸抓取（若触发）以锚点格为准整体带走。
   - 其受击/啃食矩形（`GetPlantRect`/攻击判定用的矩形）展宽到两格宽
     （~160px），身体 Reanimation 以 2.0 倍缩放锚在两格中线上（可微调现成
     的 -76/-64 偏移让"巨大"观感恰好盖住两格）。
5. 阳光：落格成功扣 200（`gPlantDefs` 的 mSeedCost）；冷却 50 秒（mRefreshTime=5000）。

### 四、本体行为与数值

- `PlantInitialize` case `SEED_GIANT_WALLNUT`：`mPlantHealth = mPlantMaxHealth = 32000`。
- 防御形态更新走坚果家族通用路径：眨眼、被啃掉血阈值、被啃表现切换。
- `AnimateNuts` 扩展巨大坚果：低于 2/3、1/3 血时给 `anim_face` 换
  WALLNUT 裂纹图（`IMAGE_REANIM_WALLNUT_CRACKED1/2`，随 2x 缩放自然放大）。
- **阻挡跳跃**：撑杆跳僵尸（vault 起跳判定）与跳跳僵尸（pogo 弹跳判定）
  在"前方障碍是巨大坚果"时不允许起跳 → 改走普通啃食路径；
  具体判定点照高坚果拦截语义接入（代码侦察后定精确函数）。
- 不攻击、不产阳光；本体只是超厚墙。

### 五、文案（properties 双语）

复用 `mPlantName` 翻译键机制（与大喷菇群一致，键放仓库翻译文件）：

| 键 | 英文（默认） | 中文（备用） |
|---|---|---|
| GIANT_WALLNUT | Giant Wall-nut | 巨大坚果 |
| GIANT_WALLNUT_TOOLTIP | A colossal nut... | （图鉴/悬浮说明：由两颗坚果融合、32000 血、挡跳跃…） |
| TRAVEL_NEED_TWO_WALLNUTS | You need two Wall-nuts side by side... | 需要把两颗紧邻的坚果作为底座。 |

> 注：坚果保龄球 2 里该种子若复用 `[GIANT_WALLNUT]` 名，顺带修正原名缺失问题
> （此前 bowling 页该名字符串缺失与否待核对，若缺失正好补上）。

### 六、存档/兼容与风险

- 复用既有枚举值，**不改任何 SeedType 值**；新增字段/成员只读不存档 →
  无存档兼容问题（挑战/旅行模式无中场存档）。
- 保龄球 2 共用 gPlantDefs 行：成本/冷却改动会影响它的传送带节奏
  （mRefreshTime 2000→5000 会让巨型球出现变慢）——如手感变差，仅对旅行路径
  覆盖数值（实现计划里给后备方案），保龄球滚球碰撞逻辑不受影响。
- 红卡着色若在部分资源包上观感不佳，可微调着色系数（纯视觉，收敛在一个函数内）。
- 两格受击依赖"僵尸选择啃食目标"逻辑 —— 需实测；若啃食只认单格，
  后备方案是让"锚点格 + 右格各放一只合体植物"改为右格放不可交互占位，
  由锚点格本体负责全部受击（实现计划选型时以代码侦察为准）。

## 增补（2026-09-06 第二轮：生存机制与入口可见性）

用户试玩反馈后增补：

### 生存/防御机制（巨大坚果）

> **统一规则：巨大坚果受到的任何伤害都"全场分摊"** —— 单次伤害总额由**场上所有巨大坚果**
> 平均承担（整除余数记在直接受击者身上，总伤害不丢）。啃食、碾压/砸击、小丑爆炸一律走
> 同一入口 `Board::GiantWallnutShareDamage`。分摊致死的**直接受击者**当场结算（计数/挑战回调），
> 远处巨大坚果若因此归零则随 `Plant::Update` 正常消失。

| 机制 | 规则 | 实现落点 |
|---|---|---|
| 自动回血 | 每 5 秒（500 tick）回复 200 HP，不超过 32000 上限（每只各自回血，不回共享池） | `Plant::Update` 巨大坚果分支 + `Plant.mGiantRegenCountdown` |
| 啃食分摊 | 僵尸每口伤害按全场巨大坚果数平分（不再只扣前排那只） | `Zombie::EatPlant` + `Board::GiantWallnutShareDamage` |
| 免疫碾压秒杀 | 被碾压/砸击不判死，伤害（合计 500）全场分摊 | `Zombie::SquishAllInSquare` 巨大坚果分支 |
| 车类僵尸击退 | 若碾压来源为**车类僵尸**（铲雪车 ZOMBIE_ZAMBONI、投石车 ZOMBIE_CATAPULT），将其向后（右）**击退一格**（≈80px，退出受击矩形避免每帧重复触发） | 同上分支（非车类如巨人砸击只扣血） |
| 免疫小丑爆炸秒杀 | 小丑僵尸/毁灭菇头爆炸（`Board::KillAllPlantsInRadius`）波及巨大坚果时不再秒杀：该次爆炸伤害（合计 2000）全场分摊；圈内其它植物照旧被清场 | `Board::KillAllPlantsInRadius` |
| 为周围植物承伤 | 以巨大坚果两格为中心的 **3 行 × 4 列**区域（行 R-1..R+1，列 C-1..C+2，排除巨大坚果自身两格）内的植物，受到**任何伤害**（啃食/碾压/爆炸/投射物）时，伤害转由巨大坚果按"全场分摊"承担，**受保护植物不掉血**；**含巨人砸地刺王的路径**（地刺王受守护时其每次 50 伤害转给巨大坚果）；范围常量在 `Board::FindGiantWallnutShield` 一处可调 | `Board::FindGiantWallnutShield` + 啃食/碾压/爆炸/投射物/巨人砸地刺王 各伤害点重定向 |

设计要点：
- 所有特殊处理只在**种植形态**（非保龄球滚球）下有意义；爆炸/碾压路径本身在保龄球关不产生，
  无需模式逃生门。
- 车类击退靠位移退出受击矩形自限频（回位后再次接触才会再扣血/再击退），不需新增每僵尸状态。
- 巨人砸击（`SquishAllInSquare` CHEW 路径）同样按碾压规则分摊 500 但不击退（非车类）。
- 场上巨大坚果越多，单只承伤越低 → "分摊"天然鼓励多种几颗；每颗仍各自 5 秒回血。

### 入口可见性排查结论"小游戏里看不到旅行体验"根因 = **运行的是旧构建**（如 `dist/` 打包目录里 8-30 的
`pvz-portable.exe`，早于 `4d080bf` 旅行入口提交）：HEAD 中 `gChallengeDefs`
（ChallengeScreen.cpp）含 `GAMEMODE_CHALLENGE_TRAVEL_1`（CHALLENGE 页 4 行 1 列，
位于斗蛐蛐右侧），且 `MoreTrophiesNeeded` 对旅行体验恒返回 0（永久可用，无奖杯门槛）。
枚举顺序与挑战数组顺序已核对一致。**修复动作 = 重新分发构建产物**
（`run-pvz.bat` 使用 `build/pvz-portable.exe`，或刷新 `dist/pvz-portable/` 与 zip）。

### 巨大坚果体验关（TRAVEL_2）定制

- 场地：**普通白天**（`BACKGROUND_1_DAY`，5 行、无池）。
- 传送带阵容：双发射手 / 机枪射手（升级自双发射手）/ 坚果 / 巨大坚果（红卡双底座）/ 樱桃炸弹。
- 出怪：普通 / 路障 / 铁桶 / 小丑（玩偶匣）/ 巨人（伽刚特尔）/ 冰车（铲雪车）；
  **6 种僵尸任何波都可能出现**（本关所有僵尸的最早出现波数统一为 1，随机不受波次限制；
  另第 5 波固定小丑+冰车、末波固定巨人作保底）；本关**自带 100 倍出怪**（倍率步骤直接 ×100，
  无需 CLI `--zombie-multiplier`，注意上限受 `MAX_ZOMBIES_IN_WAVE`=300 与僵尸池 1024 限制）。
- 旗帜：**3 面**（6 波、每 2 波一旗）。
- 音乐：传送带关音乐 `MUSIC_TUNE_CONVEYER`。
- 与 TRAVEL_1（夜间泳池·大喷菇群）并列两个旅行体验入口，均永久可用。

## 手动测试清单

1. 普通模式（冒险/生存/普通小游戏）选卡器不出现巨大坚果卡；保龄球 2 不变。
2. 旅行关获得巨大坚果（选卡页 1 或传送带）→ 卡面为**红卡**。
3. 场上单颗坚果时拖红卡 → 红禁/不可放；两颗紧邻坚果时拖到任一格 → 放行、扣 200。
4. 落格后两格变成一只巨大坚果（2x 视觉盖两格），两格都不可再种；
   旁边再放坚果不误触发。
5. 生命 32000：1/3、2/3 阈值出现裂纹脸；被僵尸啃食正常掉血。
6. 撑杆跳/跳跳僵尸碰到它不跳跃、原地啃；普通僵尸正常啃。
7. 铲子/被吃 → 两格整体消失、无残留；蹦极抓取（若可触发）整体处理。
8. 文案：英文资源包英文、中文资源包中文。
9. 回归：大喷菇群/紫卡升级/翻页/传送带、保龄球 2 滚球、普通模式种植无异常。
10. 增补机制：受伤后每 5 秒回血 200；场上种 2 颗时，啃食/碾压/爆炸伤害两只均摊
    （单只啃 4 血 → 每只 -2；碾压合计 500 → 各 -250；小丑爆炸合计 2000 → 各 -1000）；
    铲雪车/投石车碾到不掉判死并被向后击退一格（重新碾上再触发）；巨人砸击同样分摊 500（不击退）；
    小丑爆炸波及巨大坚果不死；普通坚果/其它植物对小丑爆炸与碾压行为不变。
11. 入口：用**最新构建**（`run-pvz.bat` 或刷新后的 `dist/pvz-portable`）打开小游戏页，
    第 4 行最右/斗蛐蛐右侧应有"旅行体验"入口（无需滚轮即可看到第 3 行+滚动到第 4 行）。

## 风险与取舍

- 两格占位的受击/铲除/蹦极语义是本设计改动最深的点；全部收敛在
  `SEED_GIANT_WALLNUT` 分支与种植/铲除通用路径的特判内，普通植物路径零改动。
- "双坚果底座"是一种新的种植语义（介于紫卡升级与普通种植之间）；
  实现时只扩 `CanPlantAt` 对 `SEED_GIANT_WALLNUT` 的分支，不复用紫卡 `IsUpgrade`，
  避免误伤现有升级卡逻辑。
- 手动试玩需进入旅行关：当前体验关为传送带，把巨大坚果加进传送带表即可直接验证。
