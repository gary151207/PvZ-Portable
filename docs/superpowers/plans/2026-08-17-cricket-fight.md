# 斗蛐蛐 (Cricket Fight) 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 新增小游戏「斗蛐蛐」：5 个随机植物 vs 6 个随机僵尸，纯自动单线观战，每次进入完全随机重掷。

**Architecture:** 复用 PvZ 标准挑战模式流程：新 GameMode 枚举 + 小游戏列表入口 + 定制出怪/布阵/胜负判定。植物与僵尸沿用现有精灵与行为逻辑，不新增美术资源。

**Tech Stack:** C++20, SDL2, CMake/Ninja。无自动化测试套件 — 每个任务以构建通过 + 定向手动检查为验证手段。

## Global Constraints

- **不改动 `ConstEnums.h` 现有枚举值**（序列化稳定）。新值 `GAMEMODE_CHALLENGE_CRICKET` 必须插在 `NUM_GAME_MODES` 哨兵之前、`GAMEMODE_UPSELL` 之前。
- **不新增植物/僵尸类继承**。全部改动为枚举分支（switch/else-if），遵循现有模式。
- 每个 `.h/.cpp` 文件保留现有 LGPL-3.0 许可头，不删除。
- 随机池排除清单（来自规格，技术安全原因）：
  - 植物 42 种 = `SEED_PEASHOOTER(0)` 至 `SEED_IMITATER(48)` 排除 `SEED_LILYPAD`/`SEED_TANGLEKELP`/`SEED_PUMPKINSHELL`/`SEED_FLOWERPOT`/`SEED_CATTAIL`/`SEED_COBCANNON`/`SEED_IMITATER`
  - 僵尸 20 种 = `ZOMBIE_NORMAL(0)` 至 `ZOMBIE_REDEYE_GARGANTUAR(32)` 排除 `ZOMBIE_BACKUP_DANCER`/`ZOMBIE_DUCKY_TUBE`/`ZOMBIE_SNORKEL`/`ZOMBIE_DOLPHIN_RIDER`/`ZOMBIE_BOBSLED`/`ZOMBIE_YETI`/`ZOMBIE_BOSS`/`ZOMBIE_PEA_HEAD`/`ZOMBIE_WALLNUT_HEAD`/`ZOMBIE_JALAPENO_HEAD`/`ZOMBIE_GATLING_HEAD`/`ZOMBIE_SQUASH_HEAD`/`ZOMBIE_TALLNUT_HEAD`
- 棋盘第 3 行 = 网格行索引 **2**；第 1-5 列 = 网格列索引 **0-4**。
- **判负边界**：第 3 行左侧创建 1 辆小推车（不刈草，仅作边界线），任意僵尸矩形碰到它 = 立即判负；不再有「植物全灭判负」规则。
- 构建命令：`cmake --build build`（已有配置），产物 `build/pvz-portable.exe`。

---

### Task 1: GameMode 枚举与帮助函数

**Files:**
- Modify: `src/ConstEnums.h:450-452`
- Modify: `src/LawnApp.h:270`（`IsLittleTroubleLevel` 声明附近）
- Modify: `src/LawnApp.cpp:2242-2244`（`IsLittleTroubleLevel` 实现附近）、`src/LawnApp.cpp:2296-2305`（`IsChallengeWithoutSeedBank`）

**Interfaces:**
- Produces: `GameMode::GAMEMODE_CHALLENGE_CRICKET` 枚举值；`bool LawnApp::IsCricketFightLevel()`（无参、无 board 依赖、返回 `mGameMode == GameMode::GAMEMODE_CHALLENGE_CRICKET`）。后续所有任务都依赖这两个符号。

- [ ] **Step 1: 在 ConstEnums.h 新增枚举值**

找到 `src/ConstEnums.h` 中 `GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS,` 与 `GAMEMODE_UPSELL,` 之间，插入：

```cpp
	GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS,
	GAMEMODE_CHALLENGE_CRICKET,                     // 斗蛐蛐：5 随机植物 vs 6 随机僵尸
	GAMEMODE_UPSELL,
```

- [ ] **Step 2: 在 LawnApp.h 声明帮助函数**

找到 `/*inline*/ bool IsLittleTroubleLevel();` 声明（约 270 行），在其后插入：

```cpp
	/*inline*/ bool					IsCricketFightLevel();
```

- [ ] **Step 3: 在 LawnApp.cpp 实现帮助函数**

找到 `IsLittleTroubleLevel()` 的实现（约 2242 行），在其后插入：

```cpp
bool LawnApp::IsCricketFightLevel()
{
	return mGameMode == GameMode::GAMEMODE_CHALLENGE_CRICKET;
}
```

- [ ] **Step 4: 将斗蛐蛐加入 IsChallengeWithoutSeedBank**

找到 `bool LawnApp::IsChallengeWithoutSeedBank()`（约 2296 行），在返回列表中加入 `IsCricketFightLevel() ||`：

```cpp
bool LawnApp::IsChallengeWithoutSeedBank()
{
	return 
		IsCricketFightLevel() || 
		mGameMode == GameMode::GAMEMODE_CHALLENGE_RAINING_SEEDS || 
		...
```

该函数同时驱动：种子栏不滑入、选卡界面不绘制、`GetNumSeedsInBank()` 返回 0（Start 按钮仍可点击开始）。

- [ ] **Step 5: 构建验证**

Run: `cmake --build build`
Expected: 编译通过，无错误。

- [ ] **Step 6: 提交**

```bash
git add src/ConstEnums.h src/LawnApp.h src/LawnApp.cpp
git commit -m "feat(cricket): add GAMEMODE_CHALLENGE_CRICKET enum and IsCricketFightLevel helper"
```

---

### Task 2: 小游戏列表入口

**Files:**
- Modify: `src/Lawn/Widget/ChallengeScreen.cpp:120`（`gChallengeDefs` 数组，`GAMEMODE_UPSELL` 条目之前）

**Interfaces:**
- Consumes: `GameMode::GAMEMODE_CHALLENGE_CRICKET`（Task 1）
- Produces: 小游戏页（`CHALLENGE_PAGE_CHALLENGE`）row 4 col 0 的可点击按钮「斗蛐蛐」。按钮解锁遵循标准公式（row*5+col=20 → 需要约 18 个小游戏奖杯，与 Snowy Day 同页同机制）。

- [ ] **Step 1: 在 gChallengeDefs 插入条目**

找到 `{ GameMode::GAMEMODE_UPSELL, ... }` 条目，在其前插入（icon 12 复用「小麻烦」图标，row 4 col 0 为空位）：

```cpp
	{ GameMode::GAMEMODE_CHALLENGE_CRICKET,                   12,  ChallengePage::CHALLENGE_PAGE_CHALLENGE,   4,  0,  "斗蛐蛐" },
```

注意：`GetChallengeDefinition` 断言数组索引与枚举偏移一致，因此该条目**必须**插在 UPSELL 条目之前（与 Task 1 的枚举插入位置对应）。

- [ ] **Step 2: 构建验证**

Run: `cmake --build build`
Expected: 编译通过。`GetChallengeDefinition` 断言（`TOD_ASSERT(aDef.mChallengeMode == theChallengeMode + GAMEMODE_SURVIVAL_NORMAL_STAGE_1)`）在 Release 下不触发；Debug 下若枚举/数组错位会立即暴露。

- [ ] **Step 3: 提交**

```bash
git add src/Lawn/Widget/ChallengeScreen.cpp
git commit -m "feat(cricket): add 斗蛐蛐 button to minigame challenge screen"
```

---

### Task 3: 出怪系统（一波 6 随机僵尸，全走第 3 行，开局即刷）

**Files:**
- Modify: `src/Lawn/Challenge.cpp:2531-2551`（`Challenge::InitZombieWaves`，在 `GAMEMODE_CHALLENGE_SPEED` 分支前）
- Modify: `src/Lawn/Board.cpp:630-646`（`PickZombieWaves` 波数分支）、`src/Lawn/Board.cpp:820-831`（`PickZombieWaves` 循环结束处）
- Modify: `src/Lawn/Board.cpp:5160-5176`（`SpawnZombieWave` 的 `AddZombie` 调用）
- Modify: `src/Lawn/Board.cpp:1260-1283`（`InitZombieWaves` 的 `mZombieCountDown` 分支）

**Interfaces:**
- Consumes: `IsCricketFightLevel()`（Task 1）
- Produces: `mNumWaves == 1`；`mZombiesInWave[0][0..5]` 为 6 个随机安全池僵尸、`[6]` 为 `ZOMBIE_INVALID`；僵尸经 `AddZombieInRow(type, 2, wave)` 全部落第 3 行；`mZombieCountDown == 1`（开战后 1 帧刷出）。

- [ ] **Step 1: Challenge::InitZombieWaves 允许随机池僵尸**

找到 `void Challenge::InitZombieWaves()` 中 `else if (aGameMode == GAMEMODE_CHALLENGE_SPEED)` 分支，在其前插入：

```cpp
	else if (mApp->IsCricketFightLevel())
	{
		aList[ZOMBIE_NORMAL] = true;
		aList[ZOMBIE_FLAG] = true;
		aList[ZOMBIE_TRAFFIC_CONE] = true;
		aList[ZOMBIE_POLEVAULTER] = true;
		aList[ZOMBIE_PAIL] = true;
		aList[ZOMBIE_NEWSPAPER] = true;
		aList[ZOMBIE_DOOR] = true;
		aList[ZOMBIE_FOOTBALL] = true;
		aList[ZOMBIE_DANCER] = true;
		aList[ZOMBIE_ZAMBONI] = true;
		aList[ZOMBIE_JACK_IN_THE_BOX] = true;
		aList[ZOMBIE_BALLOON] = true;
		aList[ZOMBIE_DIGGER] = true;
		aList[ZOMBIE_POGO] = true;
		aList[ZOMBIE_BUNGEE] = true;
		aList[ZOMBIE_LADDER] = true;
		aList[ZOMBIE_CATAPULT] = true;
		aList[ZOMBIE_GARGANTUAR] = true;
		aList[ZOMBIE_IMP] = true;
		aList[ZOMBIE_REDEYE_GARGANTUAR] = true;
	}
```

- [ ] **Step 2: PickZombieWaves 设置单波**

在 `PickZombieWaves()` 的波数分支链（`else if (aGameMode == GameMode::GAMEMODE_CHALLENGE_WHACK_A_ZOMBIE) mNumWaves = 12;` 之后）插入：

```cpp
		else if (mApp->IsCricketFightLevel())
			mNumWaves = 1;
```

- [ ] **Step 3: PickZombieWaves 覆盖波 0 为 6 只随机僵尸**

在 `PickZombieWaves()` 的 `for (int aWave = 0; aWave < mNumWaves; aWave++)` 循环结束的 `}` 之后、函数结束的 `}` 之前（即 `PutZombieInWave(...)` 调用后的两个闭合大括号之间）插入：

```cpp
	// 斗蛐蛐：覆盖标准随机填充，一波直接出 6 只随机僵尸（技术安全池，见规格）
	if (mApp->IsCricketFightLevel())
	{
		static const ZombieType gCricketZombiePool[] = {
			ZombieType::ZOMBIE_NORMAL, ZombieType::ZOMBIE_FLAG, ZombieType::ZOMBIE_TRAFFIC_CONE,
			ZombieType::ZOMBIE_POLEVAULTER, ZombieType::ZOMBIE_PAIL, ZombieType::ZOMBIE_NEWSPAPER,
			ZombieType::ZOMBIE_DOOR, ZombieType::ZOMBIE_FOOTBALL, ZombieType::ZOMBIE_DANCER,
			ZombieType::ZOMBIE_ZAMBONI, ZombieType::ZOMBIE_JACK_IN_THE_BOX, ZombieType::ZOMBIE_BALLOON,
			ZombieType::ZOMBIE_DIGGER, ZombieType::ZOMBIE_POGO, ZombieType::ZOMBIE_BUNGEE,
			ZombieType::ZOMBIE_LADDER, ZombieType::ZOMBIE_CATAPULT, ZombieType::ZOMBIE_GARGANTUAR,
			ZombieType::ZOMBIE_IMP, ZombieType::ZOMBIE_REDEYE_GARGANTUAR
		};
		const int aPoolSize = static_cast<int>(sizeof(gCricketZombiePool) / sizeof(gCricketZombiePool[0]));
		for (int i = 0; i < 6; i++)
		{
			mZombiesInWave[0][i] = gCricketZombiePool[Rand(aPoolSize)];
		}
		mZombiesInWave[0][6] = ZombieType::ZOMBIE_INVALID;
	}
```

（`Rand()` 已在 Board.cpp 中可用，别处已有 `Rand(100)` 用法。`IsZombieWaveDistributionOk` 仅 Adventure 模式断言，不受影响。）

- [ ] **Step 4: SpawnZombieWave 强制第 3 行**

在 `SpawnZombieWave()` 的 `else { ... AddZombie(aZombieType, mCurrentWave); }` 分支中，将 `AddZombie(aZombieType, mCurrentWave);`（约 5174 行）替换为：

```cpp
				if (mApp->IsCricketFightLevel())
					AddZombieInRow(aZombieType, 2, mCurrentWave);   // 斗蛐蛐：全部走第 3 行（中间格线）
				else
					AddZombie(aZombieType, mCurrentWave);
```

- [ ] **Step 5: InitZombieWaves 开局即刷**

在 `InitZombieWaves()` 的 `mZombieCountDown` 分支链中，`else if (mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_SNOWY_DAY)` 分支之后、最终 `else` 之前插入：

```cpp
	else if (mApp->IsCricketFightLevel())
	{
		mZombieCountDown = 1;   // 斗蛐蛐：开战后立即刷出整波
	}
```

- [ ] **Step 6: 构建验证**

Run: `cmake --build build`
Expected: 编译通过。

- [ ] **Step 7: 提交**

```bash
git add src/Lawn/Challenge.cpp src/Lawn/Board.cpp
git commit -m "feat(cricket): single-wave 6 random zombies on middle row, spawn immediately"
```

---

### Task 4: 地图与布阵（黑夜、无阳光、仅第 3 行小推车、摆 5 植物）

**Files:**
- Modify: `src/Lawn/Board.cpp:980-994`（`PickBackground` 的 `BACKGROUND_2_NIGHT` 组）
- Modify: `src/Lawn/Board.cpp:1400-1410`（`InitLevel` 的 `mSunMoney = 0` 分支）
- Modify: `src/Lawn/Board.cpp:1680-1697`（`SetupLawnMowers` 跳过列表与行循环）
- Modify: `src/Lawn/Board.cpp:1718-1722`（`StartLevel` 开头）、`src/Lawn/Board.h:371`（方法声明区）
- Create: `src/Lawn/Board.cpp` 内新增 `SetupCricketFight()`（放在 `StartLevel()` 定义附近）

**Interfaces:**
- Consumes: `IsCricketFightLevel()`（Task 1）、`Board::AddPlant(int, int, SeedType, SeedType)`（已存在，不做地面校验）
- Produces: `BackgroundType::BACKGROUND_2_NIGHT`；`mSunMoney = 0`；**仅第 3 行 1 辆小推车**；`SetupCricketFight()` 在 (0-4, 2) 摆 5 个随机植物。

- [ ] **Step 1: 黑夜背景**

在 `PickBackground()` 的 `BACKGROUND_2_NIGHT` case 组（含 `GAMEMODE_PUZZLE_I_ZOMBIE_8/9/ENDLESS` 等）中，`mBackground = BackgroundType::BACKGROUND_2_NIGHT; break;` 之前追加：

```cpp
	case GameMode::GAMEMODE_CHALLENGE_CRICKET:
```

- [ ] **Step 2: 初始阳光为 0**

在 `InitLevel()` 的初始阳光分支（`mApp->IsScaryPotterLevel() || mApp->IsWhackAZombieLevel()` 条件）中追加 `|| mApp->IsCricketFightLevel()`：

```cpp
	if (aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED || aGameMode == GameMode::GAMEMODE_CHALLENGE_BEGHOULED_TWIST ||
		mApp->IsScaryPotterLevel() || mApp->IsWhackAZombieLevel() || mApp->IsCricketFightLevel())
	{
		mSunMoney = 0;
	}
```

- [ ] **Step 3: 只创建第 3 行的小推车**

`SetupLawnMowers()` 中**不要**把斗蛐蛐加入跳过列表（否则没有判负边界）。改为在行循环条件中加入斗蛐蛐分支，只创建第 3 行（aRow == 2）的小推车：

将行循环条件：

```cpp
	for (int aRow = 0; aRow < MAX_GRID_SIZE_Y; aRow++)
	{
		if ((aGameMode == GameMode::GAMEMODE_CHALLENGE_RESODDED && aRow <= 4) || 
			(mApp->IsAdventureMode() && mLevel == 35) ||
			(!mApp->IsScaryPotterLevel() && mPlantRow[aRow] != PlantRowType::PLANTROW_DIRT))
		{
			LawnMower* aLawnMower = mLawnMowers.DataArrayAlloc();
			aLawnMower->LawnMowerInitialize(aRow);
			aLawnMower->mVisible = false;
		}
	}
```

改为在条件中追加 `(mApp->IsCricketFightLevel() && aRow == 2) ||`：

```cpp
	for (int aRow = 0; aRow < MAX_GRID_SIZE_Y; aRow++)
	{
		if ((aGameMode == GameMode::GAMEMODE_CHALLENGE_RESODDED && aRow <= 4) || 
			(mApp->IsAdventureMode() && mLevel == 35) ||
			(mApp->IsCricketFightLevel() && aRow == 2) ||   // 斗蛐蛐：仅第 3 行 1 辆小推车作为判负边界
			(!mApp->IsScaryPotterLevel() && mPlantRow[aRow] != PlantRowType::PLANTROW_DIRT))
		{
			LawnMower* aLawnMower = mLawnMowers.DataArrayAlloc();
			aLawnMower->LawnMowerInitialize(aRow);
			aLawnMower->mVisible = false;
		}
	}
```

（小推车在过场中由 `CutScene::Update` 自动置为可见并滑入 — `TimeLawnMowerStart[2] = 6200 < TimeIntro_End(13890)`，不受 Task 6 的 `mLawnMowerTime = 0` 影响。）

- [ ] **Step 4: 在 Board.h 声明 SetupCricketFight**

在 `src/Lawn/Board.h` 的 `void PickZombieWaves();` 声明附近插入：

```cpp
	void							SetupCricketFight();
```

- [ ] **Step 5: 实现 SetupCricketFight**

在 `Board::StartLevel()` 定义之前插入新方法（42 种植物池，排除 7 种技术异常种；摆放第 3 行（索引 2）第 1-5 列（索引 0-4），每格 1 个随机植物）：

```cpp
void Board::SetupCricketFight()
{
	static const SeedType gCricketPlantPool[] = {
		SeedType::SEED_PEASHOOTER, SeedType::SEED_SUNFLOWER, SeedType::SEED_CHERRYBOMB,
		SeedType::SEED_WALLNUT, SeedType::SEED_POTATOMINE, SeedType::SEED_SNOWPEA,
		SeedType::SEED_CHOMPER, SeedType::SEED_REPEATER, SeedType::SEED_PUFFSHROOM,
		SeedType::SEED_SUNSHROOM, SeedType::SEED_FUMESHROOM, SeedType::SEED_GRAVEBUSTER,
		SeedType::SEED_HYPNOSHROOM, SeedType::SEED_SCAREDYSHROOM, SeedType::SEED_ICESHROOM,
		SeedType::SEED_DOOMSHROOM, SeedType::SEED_SQUASH, SeedType::SEED_THREEPEATER,
		SeedType::SEED_JALAPENO, SeedType::SEED_SPIKEWEED, SeedType::SEED_TORCHWOOD,
		SeedType::SEED_TALLNUT, SeedType::SEED_SEASHROOM, SeedType::SEED_PLANTERN,
		SeedType::SEED_CACTUS, SeedType::SEED_BLOVER, SeedType::SEED_SPLITPEA,
		SeedType::SEED_STARFRUIT, SeedType::SEED_MAGNETSHROOM, SeedType::SEED_CABBAGEPULT,
		SeedType::SEED_KERNELPULT, SeedType::SEED_INSTANT_COFFEE, SeedType::SEED_GARLIC,
		SeedType::SEED_UMBRELLA, SeedType::SEED_MARIGOLD, SeedType::SEED_MELONPULT,
		SeedType::SEED_GATLINGPEA, SeedType::SEED_TWINSUNFLOWER, SeedType::SEED_GLOOMSHROOM,
		SeedType::SEED_WINTERMELON, SeedType::SEED_GOLD_MAGNET, SeedType::SEED_SPIKEROCK
	};
	const int aPoolSize = static_cast<int>(sizeof(gCricketPlantPool) / sizeof(gCricketPlantPool[0]));
	for (int aCol = 0; aCol < 5; aCol++)
	{
		AddPlant(aCol, 2, gCricketPlantPool[Rand(aPoolSize)]);
	}
}
```

- [ ] **Step 6: StartLevel 调用布阵**

在 `Board::StartLevel()` 中 `mChallenge->StartLevel();` 之后立即插入：

```cpp
	if (mApp->IsCricketFightLevel())
	{
		SetupCricketFight();   // 斗蛐蛐：开局摆 5 个随机植物（第 3 行第 1-5 列）
	}
```

（此时 SCENE 即将切为 PLAYING，植物放置后立即进入战斗；`mZombieCountDown = 1` 保证僵尸 1 帧后刷出。）

- [ ] **Step 7: 构建验证**

Run: `cmake --build build`
Expected: 编译通过。

- [ ] **Step 8: 提交**

```bash
git add src/Lawn/Board.h src/Lawn/Board.cpp
git commit -m "feat(cricket): night background, no sun/mowers, auto-place 5 random plants on middle row"
```

---

### Task 5: 判负判定（僵尸碰小推车）与进度条除零保护

**Files:**
- Modify: `src/Lawn/LawnMower.cpp:233-244`（`Update()` 的矩形重叠 → `MowZombie` 调用处）
- Modify: `src/Lawn/Board.cpp:5652-5653`（`UpdateProgressMeter` 的 `else if (mCurrentWave != 0)`）

**Interfaces:**
- Consumes: `IsCricketFightLevel()`（Task 1）、`Board::ZombiesWon(Zombie*)`（已存在）、`MowZombie(Zombie*)`（已存在，不调用）
- Produces: 斗蛐蛐模式下任意僵尸矩形碰到第 3 行小推车 → `ZombiesWon()` 立即判负（不刈草）；进度条避免 `mNumWaves - 1 == 0` 除零。

- [ ] **Step 1: LawnMower::Update 加入斗蛐蛐判负分支**

找到 `LawnMower::Update()` 中矩形重叠检测后的 `MowZombie(aZombie);` 调用处（约 239 行，上下文含「蹦极僵尸或已死亡的僵尸不能主动触发小推车」注释）：

```cpp
			int aOverlap = GetRectOverlap(aAttackRect, aZombieRect);
			if (aOverlap > (aZombie->mZombieType == ZombieType::ZOMBIE_BALLOON ? 20 : 0))
			{
				// 蹦极僵尸或已死亡的僵尸不能主动触发小推车
				if (mMowerState != LawnMowerState::MOWER_READY || (aZombie->mZombieType != ZombieType::ZOMBIE_BUNGEE && aZombie->mHasHead))
				{
					MowZombie(aZombie);
				}
			}
```

将 `MowZombie(aZombie);` 替换为：

```cpp
				if (mMowerState != LawnMowerState::MOWER_READY || (aZombie->mZombieType != ZombieType::ZOMBIE_BUNGEE && aZombie->mHasHead))
				{
					if (mApp->IsCricketFightLevel())
					{
						// 斗蛐蛐：僵尸碰到小推车 → 立即判负（不刈草）
						mBoard->ZombiesWon(aZombie);
						return;
					}
					MowZombie(aZombie);
				}
```

（`LawnMower` 已有 `mApp` 与 `mBoard` 成员，`ZombiesWon` 内部对入参有空指针保护，会走「僵尸获胜」结局路径并置 `BOARDRESULT_LOST`。气球僵尸沿用 >20 重叠阈值；蹦极僵尸从空中落下不会碰到小推车；矿工钻地到小推车之后由原版「进家」判定兜底。）

- [ ] **Step 2: 进度条除零保护**

将 `UpdateProgressMeter()` 中：

```cpp
	else if (mCurrentWave != 0)
```

改为：

```cpp
	else if (mCurrentWave != 0 && !mApp->IsCricketFightLevel())
```

（斗蛐蛐 `mNumWaves == 1`，`aTotalWidth / (mNumWaves - 1)` 会除零；观战模式不需要进度条。）

- [ ] **Step 3: 构建验证**

Run: `cmake --build build`
Expected: 编译通过。

- [ ] **Step 4: 提交**

```bash
git add src/Lawn/LawnMower.cpp src/Lawn/Board.cpp
git commit -m "feat(cricket): zombie touching lawnmower = instant loss; guard progress-meter div-by-zero"
```

---

### Task 6: 过场简化（跳过 Ready Set Plant 与小推车动画）

**Files:**
- Modify: `src/Lawn/CutScene.cpp:731-749`（`StartLevelIntro` 的 `mReadySetPlantTime` 分支）
- Modify: `src/Lawn/CutScene.cpp:753-755`（`mLawnMowerTime` 赋值）

**Interfaces:**
- Consumes: `IsCricketFightLevel()`（Task 1）
- Produces: 斗蛐蛐过场无 Ready Set Plant 阶段（`mReadySetPlantTime = 0`）、无小推车滚动动画（`mLawnMowerTime = 0`），过场为最短标准时长后自动进入战斗。

- [ ] **Step 1: 跳过 Ready Set Plant**

在 `StartLevelIntro()` 的 `mReadySetPlantTime = 0;` 分支条件（`mApp->IsScaryPotterLevel() ||` 附近，即 `IsShovelLevel/IsSquirrelLevel/IsWallnutBowlingLevel/ZOMBIQUARIUM/LAST_STAND/TREE_OF_WISDOM/IZombie/WhackAZombie/ScaryPotter` 列表）追加 `mApp->IsCricketFightLevel() ||`：

```cpp
	else if (mApp->IsShovelLevel() ||
		mApp->IsSquirrelLevel() ||
		mApp->IsWallnutBowlingLevel() ||
		mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_ZOMBIQUARIUM ||
		mApp->mGameMode == GameMode::GAMEMODE_CHALLENGE_LAST_STAND ||
		mApp->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM ||
		mApp->IsIZombieLevel() ||
		mApp->IsWhackAZombieLevel() ||
		mApp->IsScaryPotterLevel() ||
		mApp->IsCricketFightLevel())
	{
		mReadySetPlantTime = 0;
	}
```

- [ ] **Step 2: 跳过小推车动画**

将：

```cpp
	mLawnMowerTime = 0;
	if (!IsSurvivalRepick())
	{
		mLawnMowerTime = 550;
	}
```

改为：

```cpp
	mLawnMowerTime = 0;
	if (!IsSurvivalRepick() && !mApp->IsCricketFightLevel())
	{
		mLawnMowerTime = 550;
	}
```

- [ ] **Step 3: 构建验证**

Run: `cmake --build build`
Expected: 编译通过。

- [ ] **Step 4: 提交**

```bash
git add src/Lawn/CutScene.cpp
git commit -m "feat(cricket): skip Ready-Set-Plant and lawnmower intro animation"
```

---

### Task 7: 整体构建与手动验证

**Files:** 无代码改动。

- [ ] **Step 1: 完整构建**

Run: `cmake --build build`
Expected: 编译链接通过，产物 `build/pvz-portable.exe` 更新。

- [ ] **Step 2: 解锁快速测试入口（临时）**

斗蛐蛐按钮位于小游戏页 row 4 col 0，标准解锁公式要求约 18 个奖杯。若测试存档奖杯不足，临时将 `ChallengeScreen::MoreTrophiesNeeded` 中 `CHALLENGE_PAGE_CHALLENGE` 分支改为对 `GAMEMODE_CHALLENGE_CRICKET` 返回 0（测试后还原）；或在 Debug 构建用 `-tod` 参数。

- [ ] **Step 3: 手动验证清单（每项记录结果）**

从小游戏页进入「斗蛐蛐」后依次确认：

1. 背景为**黑夜**（`BACKGROUND_2_NIGHT`）。
2. 过场为最短时长，无 Ready Set Plant、无小推车滚动动画，结束后自动开战（无需点选卡）。
3. 开战后第 3 行（中间格线）第 1-5 列有 **5 个随机植物**，每列 1 个。
4. 第 3 行左侧有 **1 辆小推车**（其余行无），过场中正常滑入显示。
5. 第 3 行右侧出现 **6 个随机僵尸**，全部沿第 3 行走来。
6. **无种子栏、无铲子、无阳光计数**，玩家无法交互（纯观战）。
7. 再次进入小游戏列表后再进入，植物种类与僵尸种类明显变化（完全随机）。
8. 多次进入（≥5 次）观察：出现过不同植物组合（如含向日葵的弱阵）、不同僵尸组合（如含伽刚特尔/气球）。
9. 胜利路径：6 僵尸全灭 → 正常胜利结算（奖杯/奖励）。
10. 失败路径：任意僵尸碰到第 3 行小推车 → 立即判负（僵尸获胜结局）。确认小推车**不会刈草**（无割草动画/杀僵尸）。
11. 特殊僵尸兜底：若抽到气球/矿工，观察其越过小推车后由「进家」判定判负（可选验证，概率低）。
12. 胜利/失败后返回小游戏列表无异常；再进入可正常开始新一局。
13. 回归：正常小游戏（如「小麻烦」）、冒险模式第 1 关可正常游玩（确认枚举新增未破坏现有流程）。

- [ ] **Step 4: 修复发现的问题并提交**

如有问题：回到对应任务修复 → 重新构建 → 重跑清单对应项 → 单独提交修复。

---

## Self-Review 记录

- **规格覆盖**：黑夜背景（T4）、中间行布阵（T4）、单波 6 随机僵尸（T3）、全走第 3 行（T3）、纯自动无 UI（T1 IsChallengeWithoutSeedBank + T4 无阳光）、完全随机（T3/T4 随机池）、胜利 = 僵尸全灭（标准结算）、失败 = 僵尸碰小推车（T5，不刈草；进家兑底不变）、单场制（标准结算流程）、小游戏入口（T2）、过场简化（T6）。
- **占位符扫描**：无 TBD/TODO；所有插入点给出精确锚点与完整代码。
- **类型一致性**：`IsCricketFightLevel()`、`SetupCricketFight()`、`AddZombieInRow(type, 2, wave)`、`ZombiesWon(aZombie)`、`MowZombie`（不调用）在定义与调用处签名一致；Task 5 已删除 `HasAlivePlant`/植物全灭判定（用户选择 B）。
