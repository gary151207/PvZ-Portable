# 旅行模式专属红卡植物"巨大坚果"（Giant Wall-nut）实现计划

日期：2026-09-06
设计：`docs/superpowers/specs/2026-09-06-giant-wallnut-design.md`

**Goal:** 复用 `SEED_GIANT_WALLNUT`（现仅作保龄球 2 滚球）做成一张**旅行专属红卡**防御植物：
双坚果底座融合（拖到已种坚果格、同行相邻格还有一颗坚果 → 两颗合并）、占两格、
200 阳光/50s 冷却/32000 HP、阻挡撑杆跳与跳跳；红卡卡面 + 中英文案。

**Global Constraints:**
- 复用既有 `SeedType::SEED_GIANT_WALLNUT` 值（在 `NUM_SEED_TYPES` 前），**不动任何既有枚举值**。
- 不做类继承；全部按 `SEED_GIANT_WALLNUT` 枚举分支（AGENTS.md）。
- 不改普通模式行为与保龄球 2 滚球行为（分支按游戏模式/状态隔离）。
- 不加新美术：卡面红色 = 对既有卡包 cel 做红色着色；本体 = `REANIM_WALLNUT` 2x。
- 每任务提交一次；文档（spec/plan）单独提交。

---

### Task 1: 红卡卡面与旅行专属注册（判定 + 着色 + 可拥有/可获取）

**Files:**
- Modify: `src/Lawn/Travel.cpp`（`gTravelPlantDefs` 加 `{ SEED_GIANT_WALLNUT, false }`；`NUM_TRAVEL_PLANTS` 自动）
- Modify: `src/LawnApp.cpp`（`HasSeedType` ~L2692 加 `case SEED_GIANT_WALLNUT: return IsTravelLevel(mGameMode);`）
- Modify: `src/Lawn/Plant.h/.cpp`（新增静态 `bool IsRedCard(SeedType)`，对标 `IsUpgrade`）
- Modify: `src/Lawn/SeedPacket.cpp`（`DrawSeedPacket` 底色 ~L301-317 加红卡着色；`SetPacketType` ~L1130 加 5000 例外）
- Modify: `src/Lawn/Challenge.cpp`（`UpdateConveyorBelt()` 旅行关种子表 ~L1853 加 **坚果底座** 与 巨大坚果）
- Modify: `src/Lawn/Plant.cpp`（`gPlantDefs` SEED_GIANT_WALLNUT 行成本/冷却）
- Modify: `properties/pvzp-strings.xml`、`properties/pvzp-strings.zh-CN.xml`（文案）

**Interfaces:**
- Consumes: `SEED_GIANT_WALLNUT` 既有枚举/gPlantDefs 行（Task 0 零改动基线）
- Produces: 巨大坚果成为旅行专属植物：旅行关 `HasSeedType` 为真、可上选卡器页 1/传送带；
  卡面/卡包底色为红色；价格 200、冷却 50s；名字/悬浮说明中英可用

- [ ] **Step 1: `Travel.cpp` 登记（不改表结构）**

`Travel.h` 的 `TravelPlantDef{ mSeedType; bool mUpgradeCard; }` **保持不变**
（侦察确认 `mUpgradeCard` 全仓库无人读取，紫卡观感实际由 `Plant::IsUpgrade` 全局列表决定；
红卡照此模式即可，不引入冗余字段）。
`Travel.cpp` `gTravelPlantDefs[]`：
```cpp
const TravelPlantDef gTravelPlantDefs[] = {
    { SeedType::SEED_FUMESHROOM_GROUP, true  },
    { SeedType::SEED_GIANT_WALLNUT,    false },
};
```
（页 1 排布 `SeedChooserScreen.cpp:304-318` 按表内序号自动给位置；`IsTravelOnlySeed` 自动覆盖。）

- [ ] **Step 2: `LawnApp::HasSeedType` 旅行特判**

`LawnApp.cpp` `HasSeedType`（~L2692，`SEED_FUMESHROOM_GROUP` case 后）加：
```cpp
	case SeedType::SEED_GIANT_WALLNUT:
		return IsTravelLevel(mGameMode);   // 旅行专属：仅旅行关可选/拥有
```
（`SEED_GIANT_WALLNUT=50`，走 default 时 `GetSeedsAvailable()≤49` 恒 false——不特判普通模式就永远拿不到，安全。）

- [ ] **Step 3: `Plant::IsRedCard` 判定**

`Plant.h`：`static /*inline*/ bool IsRedCard(SeedType theSeedtype);`
`Plant.cpp`（`IsUpgrade` 旁）：
```cpp
bool Plant::IsRedCard(SeedType theSeedtype)
{
    return theSeedtype == SeedType::SEED_GIANT_WALLNUT;   // 目前唯一红卡（旅行专属高阶卡）
}
```
> 说明：红色是卡本身的属性（与紫卡 `IsUpgrade` 同构：观感全局一致，保龄球 2 选到巨型球也显示红卡）；
> "是否旅行专属/可拥有"仍由 `IsTravelOnlySeed` + `HasSeedType` 决定。

- [ ] **Step 4: 卡包底色红色着色**

`SeedPacket.cpp` `DrawSeedPacket` 底色选择块（~L301-309）——图集 `IMAGE_SEEDS` 没有红色 cel
（cel 0=模仿者、1=升级紫卡、2=绿卡、3-8=小游戏按钮），所以红卡 = **选中绿卡 cel(2) 后红色着色**：
- 在底色三元式处先求 `bool aIsRedCard = Plant::IsRedCard(aSeedType);`（仅非灰/暗的常规态按红处理，
  灰/暗由函数既有的 `theGrayness/thePercentDark` 通道负责，红卡叠加其上也成立）；
- 绘制底色 cel（~L317 `TodDrawImageCelScaledF(g, IMAGE_SEEDS, ...)`）时对红卡用
  `Graphics aBgG(*g)` 子画布 + `aBgG.SetColorizeImages(true); aBgG.SetColor(Color(255, 96, 96));`
  只把这一张底图染红，作用域不污染其后卡图/阳光数字（参照同函数 ~L275 子画布先例）；
  若 ~L554 处有"暗色覆盖重画同一 cel"，红卡同样走子画布染色（保持禁用态观感一致）。

- [ ] **Step 5: 数值与冷却特判**

`Plant.cpp` `gPlantDefs[]` `SEED_GIANT_WALLNUT` 行改成本与冷却：
```cpp
{ SeedType::SEED_GIANT_WALLNUT, nullptr, ReanimationType::REANIM_WALLNUT, 2, 200, 5000, PlantSubClass::SUBCLASS_NORMAL, 0, "GIANT_WALLNUT" },
```
（字段序：mSeedType, mPlantImage, mReanimationType, mPacketIndex, mSeedCost=200, mRefreshTime=5000(50s), mSubClass, mLaunchRate, mPlantName。）
`SeedPacket.cpp` `SetPacketType`（~L1126-1135）照 GATLINGPEA 例外给巨大坚果加 5000 豁免，
避免开局被通用"5000 或升级卡"分支压成 3500 + 预冷却：
```cpp
	if (aUseSeedType == SeedType::SEED_GATLINGPEA || aUseSeedType == SeedType::SEED_GIANT_WALLNUT)
	{
		// Gatling Pea / Giant Wall-nut use their own base cooldown; skip generic overrides.
	}
```
> **保龄球 2 影响分析**：传送带 `AddSeed`（SeedPacket.cpp ~L1017-1047）免冷却直接激活、用掉的传送带包
> 不原地充能（由 `Challenge::UpdateConveyorBelt` 的权重表周期补新卡），因此 `mRefreshTime` 5000
> 不影响保龄球 2 巨型球的出现节奏；成本 200 在传送带不生效。改 `mPlantHealth`（Task 3）同理不影响
> 保龄球（滚球不吃血）。

- [ ] **Step 6: 传送带可获取（底座坚果 + 巨大坚果）**

`Challenge.cpp` `UpdateConveyorBelt()` 旅行关种子表（~L1853-1869）追加两项、`aSeedPickCount` 6→8
（先读函数确认数组上限与计数变量名）：
```cpp
		aSeedPickArray[6].mItem = SeedType::SEED_WALLNUT;           // 融合底座必须的普通坚果
		aSeedPickArray[6].mWeight = 14;
		aSeedPickArray[7].mItem = SeedType::SEED_GIANT_WALLNUT;     // 红卡本体
		aSeedPickArray[7].mWeight = 6;
```
（巨大坚果红卡无底座坚果时无法落格，所以**必须同给 WALLNUT**，否则体验关拿不到底座。）

- [ ] **Step 7: 文案**

`properties/pvzp-strings.xml`（英文默认）与 `pvzp-strings.zh-CN.xml`（中文备用，同 id 一套）追加：
```xml
<String id="GIANT_WALLNUT">Giant Wall-nut</String>
<String id="GIANT_WALLNUT_TOOLTIP">A colossal nut grown by fusing two Wall-nuts side by side. 32000 toughness. Jumping zombies (pole vaulters, pogo) cannot leap over it.</String>
<String id="TRAVEL_NEED_TWO_WALLNUTS">You need two Wall-nuts side by side in the same row to fuse them.</String>
```
```xml
<String id="GIANT_WALLNUT">巨大坚果</String>
<String id="GIANT_WALLNUT_TOOLTIP">由两颗紧邻的坚果融合而成的巨型坚果：32000 生命，撑杆跳僵尸与跳跳僵尸都无法跳过它。</String>
<String id="TRAVEL_NEED_TWO_WALLNUTS">需要同一行有两颗紧邻的坚果作为底座才能融合。</String>
```
> `[GIANT_WALLNUT]`/`[GIANT_WALLNUT_TOOLTIP]` 即 `mPlantName` 生成的卡名/悬浮键（Plant.cpp
> `GetNameString`/`GetToolTip`）；若主 pak 的 LawnStrings 已含同名键会被后加载覆盖，无则本次补齐。

- [ ] **Step 8: 编译 + 冒烟**

Run: `cmake --build build`（或仓库 build 脚本）；进旅行关观察：选卡器页 1 / 传送带出现巨大坚果红卡；
普通模式无此卡；保龄球 2 卡面也显示红卡（预期内）。

- [ ] **Step 9: Commit**

```bash
git add src/Lawn/Travel.cpp src/LawnApp.cpp src/Lawn/Plant.h src/Lawn/Plant.cpp src/Lawn/SeedPacket.cpp src/Lawn/Challenge.cpp properties/pvzp-strings.xml properties/pvzp-strings.zh-CN.xml
git commit -m "feat(travel): giant wall-nut red card registry (travel-only, red packet face, 200 sun / 50s cd, conveyor pick, en/zh strings)"
```

---

### Task 2: 双坚果底座种植机制（融合式落格）

**Files:**
- Modify: `src/ConstEnums.h`（`PlantingReason` 追加 `PLANTING_NEEDS_TWO_WALLNUTS`）
- Modify: `src/Lawn/Board.cpp`（`CanPlantAt`；`MouseDownWithPlant` 提示分支；种植成功路径两坚果 `Die()` + 锚点 `AddPlant`）
- Modify: `src/Lawn/Plant.cpp`（`UpdateReanimColor`/`IsPartOfUpgradableTo` 放行高亮扩展；`IsUpgradableTo` 不动——不是紫卡）

**Interfaces:**
- Consumes: Task 1（卡可入手）
- Produces: 光标带巨大坚果卡：拖到"自身 + 同行相邻格各有一颗坚果"的格上放行（闪白）；
  落格扣除 200 阳光、两颗坚果消失、生成一只占两格的巨大坚果；不满足 → 红禁/提示

- [ ] **Step 1: 读透 `CanPlantAt` 全函数**

先读 `Board::CanPlantAt`（~L3200-3410）整体：认识 `aNormalPlant/aUnderPlant/aPumpkinPlant`、
`PLANTING_NOT_HERE/NEEDS_UPGRADE` 判定结构与各早退分支，选在坚果特判区（~L3381 "坚果包扎术"附近
或函数尾部返回 OK 前）插入巨大坚果分支，避免干扰紫卡/睡莲/花盆路径。

- [ ] **Step 2: 新种植原因**

`ConstEnums.h` `PlantingReason`（~L754-770）末尾追加：
```cpp
    PLANTING_NEEDS_TWO_WALLNUTS
```
（枚举无存档，追加安全。）

- [ ] **Step 3: `CanPlantAt` 分支**

在 `CanPlantAt` 合适位置（先经既有"坚果/花盆/紫卡"各检查通过后）加：
```cpp
	if (theSeedType == SeedType::SEED_GIANT_WALLNUT)
	{
		// 双坚果底座：目标格 + 同行相邻格各需一颗普通坚果（非被蹦极抓取）
		Plant* aTarget = aNormalPlant;   // 目标格的主层植物
		if (aTarget && aTarget->mSeedType == SeedType::SEED_WALLNUT &&
			aTarget->mOnBungeeState != PlantOnBungeeState::GETTING_GRABBED_BY_BUNGEE)
		{
			for (int aDX : { -1, 1 })
			{
				int aNX = theGridX + aDX;
				if (aNX < 0 || aNX >= MAX_GRID_SIZE_X) continue;
				PlantsOnLawn aNeighbor;
				GetPlantsOnLawn(aNX, theGridY, &aNeighbor);
				Plant* aNeighborNut = aNeighbor.mNormalPlant;
				if (aNeighborNut && aNeighborNut->mSeedType == SeedType::SEED_WALLNUT &&
					aNeighborNut->mOnBungeeState != PlantOnBungeeState::GETTING_GRABBED_BY_BUNGEE)
				{
					return PlantingReason::PLANTING_OK;
				}
			}
		}
		return PlantingReason::PLANTING_NEEDS_TWO_WALLNUTS;
	}
```
（位置与细节以读到的 `CanPlantAt` 实际结构为准：注意 `aNormalPlant` 可能被紫卡/南瓜等早退；
巨大坚果必须走"目标格有主植物"这一支；若函数在 aNormalPlant 存在时已有
"非升级卡直接 PLANTING_NOT_HERE"的早退，需把本分支放在它之前。附加硬性要求：
- 两颗坚果都不能套南瓜（`aPumpkinPlant == nullptr`），避免融合后南瓜悬空；
- 分支整体包在 `!mApp->IsWallnutBowlingLevel()` 里：保龄球 2 共用该 SeedType 滚球，
  原 `CanPlantAt` 语义必须保留（逃生门）；

- [ ] **Step 4: 失败提示**

`Board.cpp` `MouseDownWithPlant` 提示链（~L4225-4296 区）追加：
```cpp
		else if (aReason == PlantingReason::PLANTING_NEEDS_TWO_WALLNUTS && aPlantingSeedType == SeedType::SEED_GIANT_WALLNUT)
		{
			DisplayAdvice("[TRAVEL_NEED_TWO_WALLNUTS]", MessageStyle::MESSAGE_STYLE_HINT_FAST, AdviceType::ADVICE_CANT_PLANT_THERE);
		}
```
（AdviceType 复用 ADVICE_CANT_PLANT_THERE；如需专属类型再扩 AdviceType——先确认其枚举是否需要新增，
本项目倾向复用既有 type，仅换字幕键。）

- [ ] **Step 5: 落格执行（两颗合并）**

`Board` 种植成功路径（~L4398-4446 销毁旧植物区，`aNormalPlant->Die()` 前后）对巨大坚果：
```cpp
	if (aPlantingSeedType == SeedType::SEED_GIANT_WALLNUT)
	{
		// 底座两颗坚果整株替换为一只巨大坚果（锚点 = 两格中的左格）
		Plant* aTargetNut = aPlantOnLawn.mNormalPlant;   // 落格上的坚果
		if (aTargetNut) aTargetNut->Die();
		// 找到相邻那颗坚果并移除
		for (int aDX : { -1, 1 })
		{
			int aNX = aGridX + aDX;
			if (aNX < 0 || aNX >= MAX_GRID_SIZE_X) continue;
			PlantsOnLawn aNeighbor;
			GetPlantsOnLawn(aNX, aGridY, &aNeighbor);
			Plant* aNut = aNeighbor.mNormalPlant;
			if (aNut && aNut->mSeedType == SeedType::SEED_WALLNUT)
			{
				if (aNX < aGridX) { /* 邻居在左 → 锚点改左格，稍后 AddPlant 用 aNX */ }
				aNut->Die();
				break;
			}
		}
	}
```
- 锚点选择：**两颗中的左格**作 `mPlantCol`（视觉左起、受击两格矩形从左到右）。
  落格在右格时锚点 = 右格-1。实现时先在种植函数算好 `int aAnchorX` 再传给 `AddPlant`。
- 池格场景：若底座坚果种在睡莲上，融合后锚格睡莲保留（巨大坚果踩莲）、**右格残留的睡莲一并 Die**，
  保证第二格完全干净（两格占用语义一致）。
- 阳光扣除在既有"非传送带卡槽种植"路径已统一处理（`TakeSunMoney`），无需另写。
- 先读 `AddPlant(int,int,SeedType,SeedType)` 签名确认传锚点即可；保龄球 2 的落球不走此路径
  （直接由滚球逻辑 AddPlant），不受影响。

- [ ] **Step 6: 放行高亮（可融合时坚果闪白）**

`Plant::UpdateReanimColor`（~L2959-2999）的"光标种子可作用于此植物 → 闪白"分支，
在 `IsPartOfUpgradableTo(aSeedType)` 判定后补巨大坚果情形：
```cpp
	// 巨大坚果红卡：可融合（本格坚果 + 相邻坚果）时闪白
	bool aGiantNutTarget = (aSeedType == SeedType::SEED_GIANT_WALLNUT) &&
		(mSeedType == SeedType::SEED_WALLNUT) &&
		mBoard->CanPlantAt(mPlantCol, mRow, aSeedType) == PlantingReason::PLANTING_OK;
```
把 `aGiantNutTarget` 并入闪白条件（与现 upgrade 分支并列，不给普通路径加负担）。
> 只高亮落点格那颗（被融合本体）；相邻坚果是否也闪白可选（默认不高亮，避免噪点）。

- [ ] **Step 7: 编译 + 手动验证**

1. 场上单颗坚果拖红卡 → 红禁；出现提示文案（中/英）
2. 两颗紧邻坚果拖到任一格 → 放行闪白；落格扣 200、两格消失生成巨大坚果
3. 普通模式/其它模式不出现该卡（回归）
4. 空中落点/池格/屋顶无坚果处 → 正常红禁（无崩溃）

- [ ] **Step 8: Commit**

```bash
git add src/ConstEnums.h src/Lawn/Board.cpp src/Lawn/Plant.cpp src/Lawn/Plant.h
git commit -m "feat(travel): giant wall-nut two-wall-nut base planting (fuse both nuts, land on pair, red-flag guidance)"
```

---

### Task 3: 占两格本体 + 挡跳跃 + 数值表现

**Files:**
- Modify: `src/Lawn/Plant.cpp`（`PlantInitialize` HP；`GetPlantRect`/`GetPlantAttackRect` 两格宽；`AnimateNuts` 裂纹；`DoBlink`/`AttachBlinkAnim` 已含 GIANT_WALLNUT ✓；draw 偏移微调；咀嚼音效名单）
- Modify: `src/Lawn/Plant.h`（如需新增"占格宽度"辅助成员/方法）
- Modify: `src/Lawn/Zombie.cpp`（撑杆跳 ~L1828 / 海豚骑手 ~L1957 / 跳跳 ~L1556 三处 TALLNUT 判定扩展
  GIANT_WALLNUT 及其落点/破杆处理；啃咬音效/碎屑名单 ~L4992/5033）
- Modify: `src/Lawn/Board.cpp`（`GetPlantsOnLawn` ~L2561-2567 的 COBCANNON 双格占位扩到 GIANT_WALLNUT；
  `CanPlantAt` 南瓜分支 ~L3316 的 COBCANNON 排除扩到 GIANT_WALLNUT）

**Interfaces:**
- Consumes: Task 2（可种出占两格个体）
- Produces: 巨大坚果 32000 HP、占该行连续两格（受击/啃食/铲除整体化）、裂纹表现、
  撑杆跳/跳跳无法跳过（撞上破杆/落地改啃）

- [ ] **Step 1: HP**

`Plant.cpp` `PlantInitialize` case `SEED_GIANT_WALLNUT`（~L341-344）：
```cpp
	case SeedType::SEED_GIANT_WALLNUT:
		mPlantHealth = 32000;
		mBlinkCountdown = 1000 + Sexy::Rand(1000);
		break;
```
（`Plant.cpp` ~L537 `mPlantMaxHealth = mPlantHealth` 自动随动。）

- [ ] **Step 2: 两格受击矩形**

`Plant.cpp` `GetPlantRect()`（~L5702）：为 `SEED_GIANT_WALLNUT`（非保龄球上下文无碍——保龄球时
mX 随滚动变化，矩形本来就该宽）返回两格宽：
```cpp
	else if (mSeedType == SeedType::SEED_GIANT_WALLNUT)
	{
		aRect = Rect(mX + 10, mY, 160 - 20, mHeight);   // 两格宽：80*2=160
	}
```
先读该方法（含 COBCANNON 特判 ~L5713：`Rect(mX, mY, 140, 80)`），确认格子像素宽 80、
COBCANNON 的 140 是"近两格"先例——按同样思路给 160。同时核对 `mWidth` 使用方：
若全局 `mWidth` 只影响网格/绘制相关，可另加成员 `int mGridWidth` 只用于本判定（以读到的实际用法定）。

- [ ] **Step 3: 双格网格占位（禁种 + 整体铲除）**

本仓库"占两格"的唯一占位机制是 `Board::GetPlantsOnLawn()`（Board.cpp ~L2534-2602）里的
COBCANNON 特判（~L2561-2567：`mPlantCol ∈ {theGridX-1, theGridX}` 两格都返回该植物）——
种植校验/铲子/蹦极/南瓜判定全部经它查询。把该特判从 COBCANNON 扩到 GIANT_WALLNUT：
```cpp
	if (aSeedType == SeedType::SEED_COBCANNON || aSeedType == SeedType::SEED_GIANT_WALLNUT)
	{
		if (aPlant->mPlantCol < theGridX - 1 || aPlant->mPlantCol > theGridX) continue;   // 覆盖 mPlantCol 与 mPlantCol+1
	}
```
（先读该函数确认变量名与结构：锚定左格后，查询右格时 `mNormalPlant = 巨大坚果` → 普通种植/南瓜
自动"此地已有植物"拒绝；铲子点右格经 `GetTopPlantAt(DIGGING)` 也命中同一只 → 整体铲除。）
- `CanPlantAt` 南瓜分支（~L3316-3319，COBCANNON 上禁南瓜）把排除条件扩到 GIANT_WALLNUT
  （两格都不许套南瓜；右格经上条自动拒绝，左格靠本分支）。

- [ ] **Step 4: 裂纹与表现**

`Plant.cpp` `AnimateNuts()`（~L3449-3470，现只处理 WALLNUT/TALLNUT）扩展巨大坚果：
- 分支：`mSeedType == SEED_GIANT_WALLNUT` 用 WALLNUT 裂纹图
  `IMAGE_REANIM_WALLNUT_CRACKED1/2`、覆盖 `anim_face`；调用点 `Plant::Animate`（~L3916-3919）
  的坚果名单加 GIANT_WALLNUT（阈值 1/3、2/3 逻辑共用）。
- 位置微调：巨大坚果 2x、更高，裂纹脸/粒子/咀嚼位 y 向上偏移（高坚果先例 `aPosY -= 32` →
  巨大坚果约再 -40；纯视觉调参）。
- `Zombie.cpp` 名单同步（让巨大坚果与坚果/高坚果手感一致）：
  - `AnimateChewSound`（~L4992）与 `AnimateChewEffect`（~L5033）：软咀嚼 + 专属碎屑名单加
    GIANT_WALLNUT（嚼的位置按 2x 高度上调）；
  - `CanTargetPlant` 扶梯可搭名单（~L6551，WALLNUT/TALLNUT/PUMPKIN）可不加（旅行关无扶梯僵尸，
    先不加，避免语义扩张）；
  - 不做高坚果的"被咬反击"（`UpdateTallnut` ~L3405 是 TALLNUT 专属；巨大坚果保持墙果式被动）。
- `Plant::Draw`/`UpdateReanim` 2x 缩放已有（~L3073-3078）；锚点后如需盖住两格，微调
  `aOffsetX`（现 -76）——纯视觉调参，先跑起来再看。

- [ ] **Step 5: 挡跳跃（撑杆跳 + 海豚骑手 + 跳跳）**

三处"空中撞高坚果"判定全部是 `mSeedType == SEED_TALLNUT` 特判，扩展时含 GIANT_WALLNUT：
- 撑杆跳：`Zombie.cpp` `UpdateZombiePolevaulter`（~L1825-1838，vault 动画 0.6-0.7 时撞 TALLNUT → bonk
  落地于其面前、转 POST_VAULT 变普通僵尸啃食）：
```cpp
			if (aPlant && (aPlant->mSeedType == SeedType::SEED_TALLNUT || aPlant->mSeedType == SeedType::SEED_GIANT_WALLNUT))
			{
				mApp->PlayFoley(FoleyType::FOLEY_BONK);
				aJumpEnds = true;
				mApp->AddTodParticle(aPlant->mX + 60, aPlant->mY - 20, mRenderOrder + 1, ParticleEffect::PARTICLE_TALL_NUT_BLOCK);
				mZombieHeight = ZombieHeight::HEIGHT_FALLING;
				if (aPlant->mSeedType == SeedType::SEED_GIANT_WALLNUT)
					mPosX = aPlant->mX + 150;   // 巨大坚果两格宽：落其右缘（前方），避免落到墙后
				else
					mPosX = aPlant->mX;
				mPosY -= 30.0f;
			}
```
- 海豚骑手（泳池行跳浪，若巨大坚果可种在睡莲上则需挡）：`UpdateZombieDolphinRider`
  （~L1954-1967）同样扩 `|| SEED_GIANT_WALLNUT`（落地逻辑照撑杆抄）。
- 跳跳：`UpdateZombiePogo`（~L1553-1565，FORWARD_BOUNCE_2 第 70 tick 撞 TALLNUT → 砸杆 `PogoBreak`）
  扩展 `|| SEED_GIANT_WALLNUT`（`PogoBreak` 原地落地，无需改落点）。
- 粒子位置用墙的前缘（巨大坚果用 aPlant->mX + 130 左右）——微调项。
- 先读 `UpdateZombieDolphinRider`/`UpdateZombiePogo` 上下文确认 mX/mPosX 语义后再落码。

- [ ] **Step 6: 编译 + 手动验证**

1. 巨大坚果 32000 血：僵尸啃食极慢；1/3、2/3 出现裂纹
2. 撑杆跳起跳撞巨大坚果 → 空中被挡、落地于墙前方、随后原地啃
3. 跳跳僵尸弹跳撞巨大坚果 → 破杆落地成普通僵尸啃食
4. 普通僵尸/其它僵尸正常啃食两格宽身体（在右格也看得到它、能咬到）
5. 巨大坚果右格不能种植物；铲掉 → 两格整体消失
6. 保龄球 2 滚球行为不变（冒烟）

- [ ] **Step 7: Commit**

```bash
git add src/Lawn/Plant.cpp src/Lawn/Plant.h src/Lawn/Zombie.cpp src/Lawn/Board.cpp
git commit -m "feat(travel): giant wall-nut body (32000 hp, 2-cell footprint & eat rect, crack frames, blocks vault/pogo jump)"
```

---

### Task 4: 收尾（端到端验证 + 文档）

**Files:**
- Modify: `CONTEXT.md`（旅行模式小节补"红卡/巨大坚果"词汇）
- 可能微调：Task 1/2/3 涉及文件（按手动验证反馈）

- [ ] **Step 1: 完整手动回归清单（逐项打勾）**

按 spec `手动测试清单` 全部走一遍（含：普通模式无卡、旅行红卡、双底座放行/红禁、占两格整体、
32000/裂纹、挡两种跳跃、铲除整体、双语文案、保龄球 2/紫卡/翻页回归）。

- [ ] **Step 2: CONTEXT.md 词汇**

旅行模式小节补：
- **红卡 (Red Card)**：卡包底色为红色的旅行高阶卡级；当前唯一一张 = **巨大坚果**。
- **巨大坚果 (Giant Wall-nut)**：旅行专属红卡（`SEED_GIANT_WALLNUT`）：200 阳光、50s 冷却、32000 生命；
  拖到**同一行两颗紧邻坚果**之一上融合底座，占两格；撑杆跳/跳跳不可跳过。
- **双坚果底座 (Twin Wall-nut Base)**：巨大坚果的种植前提——落点格与同行相邻格各有一颗普通坚果，
  两颗消耗后生成巨大坚果。
- 关系：红卡 ⊂ 旅行专属植物（页 1/传送带）；巨大坚果 = 双坚果底座产物 = 占两格墙体。

- [ ] **Step 3: 文案/数值如有微调 → 追加 commit**

```bash
git commit -m "tweak(travel): giant wall-nut ..."
```

---

## 自审备注（编写时已核对）

- `SEED_GIANT_WALLNUT` 现引用全部为保龄球相关或通用坚果家族处理（Plant.cpp L188-191/336-344/
  2650-2800 等）；种植形态 HP 4000、2x 绘制已存在，可安全在其上加新分支。
- 僵尸对植物的目标选取/啃食是**矩形碰撞制**（`Zombie::FindPlantTarget` 按
  `GetRectOverlap(attackRect, GetPlantRect()) ≥ 20` 全行扫描，Zombie.cpp ~L6588-6606）→
  巨大坚果把 `GetPlantRect` 展宽两格即可被整墙正确咬到；撑杆跳/跳跳在空中对高坚果的
  "撞墙"判定在 Zombie.cpp L1828 / L1556（TALLNUT 特判）→ 扩展巨大坚果即可挡跳跃。
- 种植判定/执行/提示链路已核对：`CanPlantAt`（Board.cpp ~L3200-3410）、失败字幕
  `MouseDownWithPlant`（~L4225-4296）、成功路径旧植物销毁（~L4398-4446）。
- 卡面底色集中在一个绘制函数 `DrawSeedPacket`（SeedPacket.cpp L282，银行/传送带/选卡器页 1/
  图鉴/奖励全部经它）→ 红卡只需改这一处底色分支（图集无红 cel，用子画布红色着色 cel 2）。
- 卡名/悬浮键由 `gPlantDefs[].mPlantName` 生成（`[GIANT_WALLNUT]`/`[GIANT_WALLNUT_TOOLTIP]`），
  文案放仓库 properties 双语文件（英文默认 + zh-CN 备用同 id），启动已加载（LawnApp.cpp ~L1918）。
- 存档兼容：复用既有枚举值（GIANT_WALLNUT=50、EXPLODE_O_NUT=49 已在 `NUM_SEED_TYPES` 前）、
  不加存档字段 → 无破坏。
- 侦察已闭环的落点（三条只读侦察 + 直接核对）：
  - 挡跳跃 = Zombie.cpp 三处 TALLNUT 空中特判：撑杆 `UpdateZombiePolevaulter` ~L1828、
    海豚 `UpdateZombieDolphinRider` ~L1957、跳跳 `UpdateZombiePogo` ~L1556；
  - 占两格 = `Board::GetPlantsOnLawn` ~L2561-2567 COBCANNON 双格特判 + `Plant::GetPlantRect`
    ~L5713 宽矩形（僵尸啃食/砸压全矩形制，无需僵尸侧新逻辑）；
  - 底座的"种在已种植物上"语义不能复用 `Plant::IsUpgrade`（会拦死保龄球 2），
    双坚果判定独立写于 `CanPlantAt` 且包 `!IsWallnutBowlingLevel()` 逃生门；
  - 冷却 5000 会被 `SeedPacket::SetPacketType` ~L1130 通用分支压成 3500 → 照 GATLINGPEA 加例外；
  - 传送带 `AddSeed`（~L1017）免冷却、用掉的包不原地充能 → 保龄球 2/旅行传送带节奏不受
    mRefreshTime/cost 影响，仅银行冷却（未来自由选卡旅行关）生效。
