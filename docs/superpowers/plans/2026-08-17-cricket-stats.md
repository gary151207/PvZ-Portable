# 斗蛐蛐胜率统计实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为无尽斗蛐蛐添加植物/僵尸胜率统计：结算时按类型聚合胜/负，持久化到 `userdata/cricket_stats.txt`，战斗中 Tab 键查看面板。

**Architecture:** 数据存 LawnApp 级（跨 Board 生命周期）；Board 开战时记录本场阵容、结算时聚合到 LawnApp 数组并写文件；面板为 Board::Draw 末尾的纯 overlay 绘制，Tab 三态循环 + 方向键/滚轮滚动。

**Tech Stack:** C++20, SDL2, CMake/Ninja。无自动化测试套件 — 每任务以构建通过 + 定向检查为验证手段。

## Global Constraints

- **不改动 `ConstEnums.h` 现有枚举值**。
- 统计数组按枚举哨兵尺寸（`SeedType::NUM_SEED_TYPES` / `ZombieType::NUM_ZOMBIE_TYPES`），不硬编码 42/20。
- 文件路径统一用 `GetAppDataPath("userdata/cricket_stats.txt")`（`using namespace Sexy` 已由 `LawnApp.h:59` 提供，可非限定调用）。
- 构建命令：`export PATH="/c/msys64/ucrt64/bin:$PATH" && cmake --build build`。

---

### Task 1: LawnApp 统计数组与文件持久化

**Files:**
- Modify: `src/LawnApp.h:122`（`BoardResult mBoardResult;` 成员附近）、`src/LawnApp.h:270`（`IsCricketFightLevel` 声明附近）
- Modify: `src/LawnApp.cpp:20-22`（include 区，`#include <time.h>` 附近）、`src/LawnApp.cpp:1236`（`LawnApp::Init`，函数末尾）

**Interfaces:**
- Produces: `int mCricketPlantWins[SeedType::NUM_SEED_TYPES]`、`mCricketPlantLosses[...]`、`mCricketZombieWins[ZombieType::NUM_ZOMBIE_TYPES]`、`mCricketZombieLosses[...]`（LawnApp 公有成员）；`void LawnApp::LoadCricketStats()`、`void LawnApp::SaveCricketStats()`。Task 2/3 依赖这些符号。

- [x] **Step 1: include `<fstream>`**

在 `src/LawnApp.cpp` 的 include 区（`#include <time.h>` 之后）插入：

```cpp
#include <fstream>
```

- [x] **Step 2: LawnApp.h 声明统计成员与方法**

在 `src/LawnApp.h` 中 `BoardResult mBoardResult;`（约 122 行）之后插入：

```cpp
	int								mCricketPlantWins[SeedType::NUM_SEED_TYPES];
	int								mCricketPlantLosses[SeedType::NUM_SEED_TYPES];
	int								mCricketZombieWins[ZombieType::NUM_ZOMBIE_TYPES];
	int								mCricketZombieLosses[ZombieType::NUM_ZOMBIE_TYPES];
```

在 `IsCricketFightLevel();` 声明（约 271 行）之后插入：

```cpp
	void							LoadCricketStats();
	void							SaveCricketStats();
```

- [x] **Step 3: LawnApp.cpp 实现加载与保存**

在 `IsCricketFightLevel()` 实现（`return mGameMode == GameMode::GAMEMODE_CHALLENGE_CRICKET;`）之后插入：

```cpp
void LawnApp::LoadCricketStats()
{
	memset(mCricketPlantWins, 0, sizeof(mCricketPlantWins));
	memset(mCricketPlantLosses, 0, sizeof(mCricketPlantLosses));
	memset(mCricketZombieWins, 0, sizeof(mCricketZombieWins));
	memset(mCricketZombieLosses, 0, sizeof(mCricketZombieLosses));

	std::ifstream aFile(GetAppDataPath("userdata/cricket_stats.txt"));
	if (!aFile.is_open())
		return;

	std::string aName;
	int aWins = 0, aLosses = 0;
	while (aFile >> aName >> aWins >> aLosses)
	{
		for (int i = 0; i < SeedType::NUM_SEED_TYPES; i++)
		{
			if (aName == GetPlantDefinition((SeedType)i).mPlantName)
			{
				mCricketPlantWins[i] = aWins;
				mCricketPlantLosses[i] = aLosses;
				break;
			}
		}
		for (int i = 0; i < ZombieType::NUM_ZOMBIE_TYPES; i++)
		{
			if (aName == GetZombieDefinition((ZombieType)i).mZombieName)
			{
				mCricketZombieWins[i] = aWins;
				mCricketZombieLosses[i] = aLosses;
				break;
			}
		}
	}
}

void LawnApp::SaveCricketStats()
{
	std::ofstream aFile(GetAppDataPath("userdata/cricket_stats.txt"));
	if (!aFile.is_open())
		return;

	for (int i = 0; i < SeedType::NUM_SEED_TYPES; i++)
	{
		aFile << GetPlantDefinition((SeedType)i).mPlantName << " "
			<< mCricketPlantWins[i] << " " << mCricketPlantLosses[i] << "\n";
	}
	for (int i = 0; i < ZombieType::NUM_ZOMBIE_TYPES; i++)
	{
		aFile << GetZombieDefinition((ZombieType)i).mZombieName << " "
			<< mCricketZombieWins[i] << " " << mCricketZombieLosses[i] << "\n";
	}
}
```

（`GetAppDataPath` 经 `LawnApp.h → SexyAppBase.h → Common.h` 可用；`GetPlantDefinition`/`GetZombieDefinition` 由 Plant.h/Zombie.h 提供，LawnApp.cpp 已包含。）

- [x] **Step 4: Init 中加载**

在 `LawnApp::Init()` 函数末尾（`ReanimatorLoadDefinitions(gLawnReanimationArray, ReanimationType::NUM_REANIMS);` 附近，或函数结束前）插入：

```cpp
	LoadCricketStats();
```

- [x] **Step 5: 构建验证**

Run: `export PATH="/c/msys64/ucrt64/bin:$PATH" && cmake --build build`
Expected: 编译链接通过。

- [x] **Step 6: 提交**

```bash
git add src/LawnApp.h src/LawnApp.cpp
git commit -m "feat(cricket): win-rate stats arrays with file persistence"
```

---

### Task 2: Board 记录本场阵容与结算聚合

**Files:**
- Modify: `src/Lawn/Board.h:371`（`SetupCricketFight` 声明附近）
- Modify: `src/Lawn/Board.cpp`（`SetupCricketFight`、`PickZombieWaves` 斗蛐蛐覆盖块、`ZombiesWon` 斗蛐蛐分支）
- Modify: `src/Lawn/Zombie.cpp`（`TrySpawnLevelAward` 斗蛐蛐分支）

**Interfaces:**
- Consumes: Task 1 的 LawnApp 统计数组与 `SaveCricketStats()`
- Produces: `Board::mCricketBattlePlants[5]`、`Board::mCricketBattleZombies[6]`（本场阵容快照）；`void Board::RecordCricketMatchResult(bool thePlantsWon)`。

- [x] **Step 1: Board.h 声明阵容与记录方法**

在 `void SetupCricketFight();` 声明（约 372 行）附近插入：

```cpp
	void							RecordCricketMatchResult(bool thePlantsWon);
	SeedType						mCricketBattlePlants[5];
	ZombieType						mCricketBattleZombies[6];
```

- [x] **Step 2: SetupCricketFight 记录植物阵容**

将 `Board::SetupCricketFight()` 末尾的布阵循环：

```cpp
	const int aPoolSize = static_cast<int>(sizeof(gCricketPlantPool) / sizeof(gCricketPlantPool[0]));
	for (int aCol = 0; aCol < 5; aCol++)
	{
		AddPlant(aCol, 2, gCricketPlantPool[Rand(aPoolSize)]);
	}
```

改为：

```cpp
	const int aPoolSize = static_cast<int>(sizeof(gCricketPlantPool) / sizeof(gCricketPlantPool[0]));
	for (int aCol = 0; aCol < 5; aCol++)
	{
		SeedType aSeedType = gCricketPlantPool[Rand(aPoolSize)];
		mCricketBattlePlants[aCol] = aSeedType;   // 记录本场植物阵容
		AddPlant(aCol, 2, aSeedType);
	}
```

- [x] **Step 3: PickZombieWaves 记录僵尸阵容**

将 `PickZombieWaves()` 斗蛐蛐覆盖块的循环：

```cpp
		for (int i = 0; i < 6; i++)
		{
			mZombiesInWave[0][i] = gCricketZombiePool[Rand(aPoolSize)];
		}
		mZombiesInWave[0][6] = ZombieType::ZOMBIE_INVALID;
```

改为：

```cpp
		for (int i = 0; i < 6; i++)
		{
			ZombieType aZombieType = gCricketZombiePool[Rand(aPoolSize)];
			mCricketBattleZombies[i] = aZombieType;   // 记录本场僵尸阵容
			mZombiesInWave[0][i] = aZombieType;
		}
		mZombiesInWave[0][6] = ZombieType::ZOMBIE_INVALID;
```

- [x] **Step 4: 实现 RecordCricketMatchResult**

在 `Board::RestartCricketMatch()` 定义之后插入：

```cpp
void Board::RecordCricketMatchResult(bool thePlantsWon)
{
	for (int i = 0; i < 5; i++)
	{
		SeedType aSeedType = mCricketBattlePlants[i];
		if (thePlantsWon)
			mApp->mCricketPlantWins[aSeedType]++;
		else
			mApp->mCricketPlantLosses[aSeedType]++;
	}
	for (int i = 0; i < 6; i++)
	{
		ZombieType aZombieType = mCricketBattleZombies[i];
		if (thePlantsWon)
			mApp->mCricketZombieLosses[aZombieType]++;
		else
			mApp->mCricketZombieWins[aZombieType]++;
	}
	mApp->SaveCricketStats();
}
```

- [x] **Step 5: 胜利分支记录**

在 `src/Lawn/Zombie.cpp` 的 `TrySpawnLevelAward` 斗蛐蛐分支（含 `// 斗蛐蛐：胜利后不落奖杯币，直接进入结算（约 2 秒停留）→ 下一场` 注释处）中，在 `mBoard->FadeOutLevel();` 之前插入：

```cpp
		mBoard->RecordCricketMatchResult(true);
```

- [x] **Step 6: 失败分支记录**

在 `Board::ZombiesWon()` 的斗蛐蛐分支（`if (mNextSurvivalStageCounter == 0) { mNextSurvivalStageCounter = 150; }` 之前）插入：

```cpp
		RecordCricketMatchResult(false);
```

- [x] **Step 7: 构建验证**

Run: `export PATH="/c/msys64/ucrt64/bin:$PATH" && cmake --build build`
Expected: 编译链接通过。

- [x] **Step 8: 提交**

```bash
git add src/Lawn/Board.h src/Lawn/Board.cpp src/Lawn/Zombie.cpp
git commit -m "feat(cricket): record battle roster and aggregate win-rate on settle"
```

---

### Task 3: Tab 面板交互与绘制

**Files:**
- Modify: `src/Lawn/Board.h:285-287`（`KeyDown` 声明附近）、`src/Lawn/Board.h:372`（方法区）
- Modify: `src/Lawn/Board.cpp`（`Board::Draw` 末尾、`Board::KeyDown` 分支链、新增 `MouseWheel`/`DrawCricketStatsPanel`）

**Interfaces:**
- Consumes: Task 1 统计数组、Task 2 无（面板只读统计）
- Produces: `Board::mCricketStatsPanel`（0=关, 1=植物, 2=僵尸）、`Board::mCricketStatsScroll`；`void Board::MouseWheel(int)` 重载；`void Board::DrawCricketStatsPanel(Graphics*)`。

- [x] **Step 1: include `<vector>`**

在 `src/Lawn/Board.cpp` 的 include 区（`#include <algorithm>` 之后）插入：

```cpp
#include <vector>
```

- [x] **Step 2: Board.h 声明面板成员与重载**

在 `virtual void KeyDown(KeyCode theKey);` 声明（约 287 行）附近插入：

```cpp
	virtual void					MouseWheel(int theDelta);
```

在 `void SetupCricketFight();` 声明（约 372 行）附近插入：

```cpp
	void							DrawCricketStatsPanel(Graphics* g);
	int								mCricketStatsPanel;   // 0=关, 1=植物, 2=僵尸
	int								mCricketStatsScroll;
```

- [x] **Step 3: KeyDown 加入 Tab 与方向键**

在 `Board::KeyDown()` 的 else-if 分支链中，`else if (theKey == KeyCode::KEYCODE_ESCAPE)` 分支**之前**插入：

```cpp
	else if (theKey == KeyCode::KEYCODE_TAB && mApp->IsCricketFightLevel())
	{
		// 斗蛐蛐胜率面板：关闭 → 植物 → 僵尸 → 关闭 循环
		mCricketStatsPanel++;
		if (mCricketStatsPanel > 2)
			mCricketStatsPanel = 0;
		mCricketStatsScroll = 0;
	}
	else if ((theKey == KeyCode::KEYCODE_UP || theKey == KeyCode::KEYCODE_DOWN) &&
		mCricketStatsPanel != 0 && mApp->IsCricketFightLevel())
	{
		mCricketStatsScroll += (theKey == KeyCode::KEYCODE_UP) ? -1 : 1;
		if (mCricketStatsScroll < 0)
			mCricketStatsScroll = 0;
	}
```

- [x] **Step 4: 实现 MouseWheel**

在 `Board::KeyDown` 实现附近（或任意 Board 方法之后）新增：

```cpp
void Board::MouseWheel(int theDelta)
{
	if (mCricketStatsPanel != 0 && mApp->IsCricketFightLevel())
	{
		mCricketStatsScroll -= theDelta;
		if (mCricketStatsScroll < 0)
			mCricketStatsScroll = 0;
	}
}
```

- [x] **Step 5: 实现 DrawCricketStatsPanel**

在 `DrawCricketStatsPanel` 声明对应的实现位置（建议放在 `Board::Draw` 定义之前）新增：

```cpp
void Board::DrawCricketStatsPanel(Graphics* g)
{
	if (mCricketStatsPanel == 0 || !mApp->IsCricketFightLevel())
		return;

	struct StatsEntry
	{
		std::string mName;
		int mWins;
		int mLosses;
		int mRate;
	};

	const int aListCount = (mCricketStatsPanel == 1) ? SeedType::NUM_SEED_TYPES : ZombieType::NUM_ZOMBIE_TYPES;
	std::vector<StatsEntry> aEntries;
	int aTotal = 0;
	for (int i = 0; i < aListCount; i++)
	{
		int aWins = (mCricketStatsPanel == 1) ? mApp->mCricketPlantWins[i] : mApp->mCricketZombieWins[i];
		int aLosses = (mCricketStatsPanel == 1) ? mApp->mCricketPlantLosses[i] : mApp->mCricketZombieLosses[i];
		if (aWins + aLosses <= 0)
			continue;
		aTotal += aWins + aLosses;
		std::string aName = (mCricketStatsPanel == 1)
			? Plant::GetNameString((SeedType)i)
			: GetZombieDefinition((ZombieType)i).mZombieName;
		aEntries.push_back({ aName, aWins, aLosses, aWins * 100 / (aWins + aLosses) });
	}
	std::stable_sort(aEntries.begin(), aEntries.end(),
		[](const StatsEntry& a, const StatsEntry& b) { return a.mRate > b.mRate; });

	const int aMaxLines = 22;
	const int aLineHeight = 16;
	const int aPanelX = 20, aPanelY = 50;
	const int aPanelW = 400;
	const int aPanelH = aMaxLines * aLineHeight + 44;
	if (mCricketStatsScroll > static_cast<int>(aEntries.size()) - aMaxLines)
		mCricketStatsScroll = std::max(0, static_cast<int>(aEntries.size()) - aMaxLines);

	g->SetColor(Color(0, 0, 0, 200));
	g->FillRect(aPanelX, aPanelY, aPanelW, aPanelH);

	std::string aTitle = (mCricketStatsPanel == 1)
		? StrFormat("PLANT WIN RATES (%d games)", aTotal)
		: StrFormat("ZOMBIE WIN RATES (%d games)", aTotal);
	TodDrawString(g, aTitle, aPanelX + aPanelW / 2, aPanelY + 22, Sexy::FONT_BRIANNETOD12, Color::White, DS_ALIGN_CENTER);
	TodDrawString(g, "[Tab] switch list    [Up/Down/Wheel] scroll", aPanelX + 8, aPanelY + 38, Sexy::FONT_BRIANNETOD12, Color(200, 200, 200), DS_ALIGN_LEFT);

	int aY = aPanelY + 54;
	for (int i = mCricketStatsScroll; i < static_cast<int>(aEntries.size()) && i < mCricketStatsScroll + aMaxLines; i++)
	{
		StatsEntry& aEntry = aEntries[i];
		std::string aLine = StrFormat("%s  %d%% (%d/%d)", aEntry.mName.c_str(), aEntry.mRate, aEntry.mWins, aEntry.mWins + aEntry.mLosses);
		TodDrawString(g, aLine, aPanelX + 8, aY, Sexy::FONT_BRIANNETOD12, Color::White, DS_ALIGN_LEFT);
		aY += aLineHeight;
	}
}
```

（`Color(0,0,0,200)` 半透明黑底；`FONT_BRIANNETOD12` 已在 ChallengeScreen 使用；`StrFormat`/`TodDrawString`/`DS_ALIGN_*` 均为项目现有工具。**面板标题/提示用 ASCII 英文** — 英文原版字体（Briannetod）无 CJK 字形，中文会渲染为空白；行内植物名走 `GetNameString`（按用户语言属性本地化，EN 版为英文），僵尸名走内部名（ASCII）。）

- [x] **Step 6: Board::Draw 末尾调用面板**

将 `Board::Draw` 末尾：

```cpp
	mDrawCount++;
	DrawGameObjects(g);
}
```

改为：

```cpp
	mDrawCount++;
	DrawGameObjects(g);
	DrawCricketStatsPanel(g);
}
```

- [x] **Step 7: 构建验证**

Run: `export PATH="/c/msys64/ucrt64/bin:$PATH" && cmake --build build`
Expected: 编译链接通过。

- [x] **Step 8: 提交**

```bash
git add src/Lawn/Board.h src/Lawn/Board.cpp
git commit -m "feat(cricket): Tab stats panel (plant/zombie win rates, scrollable)"
```

---

### Task 4: 构建与手动验证

**Files:** 无代码改动。

- [x] **Step 1: 完整构建**

Run: `export PATH="/c/msys64/ucrt64/bin:$PATH" && cmake --build build`
Expected: 编译链接通过，产物 `build/pvz-portable.exe` 更新。

- [x] **Step 2: 手动验证清单（每项记录结果）**

1. 进入斗蛐蛐打若干场（含胜利与失败），退出游戏后检查 `C:/Users/<用户>/AppData/Roaming/io.github.wszqkzqk/PvZPortable/userdata/cricket_stats.txt`：每行 `类型名 胜 负`，数值与场次一致（如某场植物胜 → 该场 5 植物各 +1 胜、6 僵尸各 +1 负）。
2. 重启游戏再打几场 → 文件数值在旧数据上累计（跨会话持久化）。
3. 战斗中按 Tab：面板在 关闭 → 植物列表 → 僵尸列表 → 关闭 间循环。
4. 面板只显示有出场记录的类型，按胜率降序，行格式 `名称 胜率% (胜/出场)`。
5. ↑/↓ 方向键与鼠标滚轮可滚动长列表（植物 42 种时验证滚动）。
6. 非斗蛐蛐模式（如冒险）按 Tab 无反应（回归）。
7. 胜负结算仍正常跳转下一场（统计记录不干扰无尽流程）。

---

## Self-Review 记录

- **规格覆盖**：数据模型（T1）、记录时机/阵容快照（T2）、持久化文件与格式（T1）、Tab 三态面板（T3）、滚动（T3）、排序与过滤（T3）、启动加载（T1）、不暂停/纯 overlay（T3 设计）、非斗蛐蛐回归（T3 的 `IsCricketFightLevel` 门控 + T4 清单）。
- **占位符扫描**：无 TBD/TODO；所有插入点给出精确锚点与完整代码。
- **类型一致性**：`RecordCricketMatchResult(bool)`、`mCricketBattlePlants[5]`、`mCricketStatsPanel`、`MouseWheel(int)`、`DrawCricketStatsPanel(Graphics*)` 在定义与调用处签名一致；`mApp->mCricketPlantWins` 等与 Task 1 成员名一致。
