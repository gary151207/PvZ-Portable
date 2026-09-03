# 旅行模式体验关（Travel Experience: Puff-shroom Group!）实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 数据驱动的旅行模式子系统骨架 + 一个可玩的旅行体验关（夜间泳池、2 旗帜、教学友好）+ 新紫卡植物"大喷菇群"（0 阳光、30 秒冷却、升级大喷菇成三头）+ 选卡器翻页（旅行关专属）。

**Architecture:** 新增 `src/Lawn/Travel.h/.cpp`（TravelLevelDef/TravelPlantDef 静态表 + `IsTravelLevel`/`GetTravelLevelDef`/`IsTravelOnlySeed` 查询 API，唯一数据入口）；`GameMode`/`SeedType` 枚举前插新值（不动既有值，保存档稳定）；Board/LawnApp/SeedChooserScreen 只通过查询 API 取旅行参数。植物升级复用既有紫卡框架（`Plant::IsUpgrade`/`IsUpgradableTo` + `Board::CanPlantAt`/种植替换），卡片只通过"升级卡"通用路径出现；两侧孢子的非锁行斜飞复用既有 `mVelX/mVelY` 角度弹道机制。

**Tech Stack:** C++20、CMake(Ninja)、SDL2。无自动化测试框架——每任务以"编译通过 + 明确的 GUI 手动检查点"为验收。

**Spec:** `docs/superpowers/specs/2026-09-03-travel-experience-design.md`

## Global Constraints

- **枚举值前插，绝不重排/复用既有值**（`ConstEnums.h`）：`GameMode` 新值加在 `NUM_GAME_MODES` 前；`SeedType` 新值加在 `NUM_SEED_TYPES` 前。存档兼容依赖此规则。
- **禁止对植物/僵尸做类继承**：一切植物行为走 `switch/if (mSeedType == ...)` 枚举分支（项目铁律）。
- **License header**：所有新建 `.h/.cpp` 必须带 LGPL-3.0-or-later 标准头（复制现有文件头）。
- **仓库无版权素材**：本实现零新美术，全部复用现有 `ReanimationType`/图像/粒子。
- **构建命令**：`./build.sh`（内部 `cmake -G Ninja -B build -DBUILD_STATIC=ON && cmake --build build`；`file(GLOB src/Lawn/*.cpp)` 每次 configure 重新收集，新建 .cpp 会自动纳入）。运行：`./run-pvz.sh`（GUI，人工操作）。
- **文案键加 `TRAVEL_` 前缀**，值放在仓库 `properties/pvzp-strings.xml`（英文默认）与 `properties/pvzp-strings.zh-CN.xml`（中文备用）。运行时读取的是 `-resdir/properties/` 磁盘副本（见 Task 3）。
- 编译日志可能很长，构建失败时看最后一个报错文件与行号即可。

---

### Task 1: 旅行子系统骨架（Travel.h/.cpp + 枚举前插）

**Files:**
- Modify: `src/ConstEnums.h`（`GameMode` 枚举区 ~L379-453；`SeedType` 枚举区 ~L1041-1125）
- Create: `src/Lawn/Travel.h`
- Create: `src/Lawn/Travel.cpp`

**Interfaces:**
- Produces（后续所有任务依赖，签名固定）：
  - `struct TravelLevelDef { int mTravelId; GameMode mGameMode; int mNumFlags; int mTotalWaves; bool mFreeChooser; bool mChooserPaged; };`
  - `struct TravelPlantDef { SeedType mSeedType; bool mUpgradeCard; };`
  - `bool IsTravelLevel(GameMode theGameMode);`
  - `const TravelLevelDef& GetTravelLevelDef(GameMode theGameMode);`
  - `bool IsTravelOnlySeed(SeedType theSeedType);`
  - `GameMode GAMEMODE_CHALLENGE_TRAVEL_1`（枚举值）；`SeedType SEED_FUMESHROOM_GROUP`（枚举值）

- [ ] **Step 1: 枚举前插**

`src/ConstEnums.h` `GameMode`：在
```cpp
    GAMEMODE_CHALLENGE_CRICKET,                     // 斗蛐蛐：5 随机植物 vs 6 随机僵尸
```
之后、`GAMEMODE_UPSELL,` 之前插入：
```cpp
    GAMEMODE_CHALLENGE_TRAVEL_1,                    // 旅行模式体验关
```

`SeedType`：在
```cpp
    SEED_LEFTPEATER,
    NUM_SEED_TYPES,
```
之间插入：
```cpp
    SEED_FUMESHROOM_GROUP,                          // 大喷菇群（紫卡升级：大喷菇 → 三头）
```

- [ ] **Step 2: 新建 `src/Lawn/Travel.h`**

完整内容（含 license 头，参照 `src/Lawn/Challenge.h` 头格式）：
```cpp
#ifndef __TRAVEL_H__
#define __TRAVEL_H__

#include "../../ConstEnums.h"

// 旅行模式数据驱动子系统：关卡表 + 旅行专属植物表。
// 后续完整旅行模式（路线推进/进度存档）只扩展本文件表与查询，不改 Board/LawnApp 模式分支。

struct TravelLevelDef
{
    int        mTravelId;        // 旅行内部关卡号（将来 1..N 沿路线推进）
    GameMode   mGameMode;        // 映射的 GameMode
    int        mNumFlags;        // 旗帜波数
    int        mTotalWaves;      // 总波数
    bool       mFreeChooser;     // 自由选卡
    bool       mChooserPaged;    // 选卡器启用翻页（旅行专属）
};

struct TravelPlantDef
{
    SeedType   mSeedType;        // 旅行专属植物
    bool       mUpgradeCard;     // 是紫卡升级卡
};

extern const TravelLevelDef gTravelLevelDefs[];
extern const int NUM_TRAVEL_LEVELS;
extern const TravelPlantDef gTravelPlantDefs[];
extern const int NUM_TRAVEL_PLANTS;

bool IsTravelLevel(GameMode theGameMode);
const TravelLevelDef& GetTravelLevelDef(GameMode theGameMode);
bool IsTravelOnlySeed(SeedType theSeedType);

#endif
```

- [ ] **Step 3: 新建 `src/Lawn/Travel.cpp`**

数据（本版 1 关 1 植物）：
```cpp
#include "Travel.h"

const TravelLevelDef gTravelLevelDefs[] = {
    // mTravelId=1：体验关。夜间泳池 2 旗帜 6 波（第 1、6 波为旗帜波），自由选卡 + 可翻页
    { 1, GameMode::GAMEMODE_CHALLENGE_TRAVEL_1, 2, 6, true, true },
};
const int NUM_TRAVEL_LEVELS = sizeof(gTravelLevelDefs) / sizeof(gTravelLevelDefs[0]);

const TravelPlantDef gTravelPlantDefs[] = {
    { SeedType::SEED_FUMESHROOM_GROUP, true },
};
const int NUM_TRAVEL_PLANTS = sizeof(gTravelPlantDefs) / sizeof(gTravelPlantDefs[0]);

bool IsTravelLevel(GameMode theGameMode)
{
    for (int i = 0; i < NUM_TRAVEL_LEVELS; i++)
        if (gTravelLevelDefs[i].mGameMode == theGameMode)
            return true;
    return false;
}

const TravelLevelDef& GetTravelLevelDef(GameMode theGameMode)
{
    for (int i = 0; i < NUM_TRAVEL_LEVELS; i++)
        if (gTravelLevelDefs[i].mGameMode == theGameMode)
            return gTravelLevelDefs[i];
    TOD_ASSERT(false); // 非旅行关卡不应调用
    return gTravelLevelDefs[0];
}

bool IsTravelOnlySeed(SeedType theSeedType)
{
    for (int i = 0; i < NUM_TRAVEL_PLANTS; i++)
        if (gTravelPlantDefs[i].mSeedType == theSeedType)
            return true;
    return false;
}
```

- [ ] **Step 4: 编译验证**

Run: `./build.sh`
Expected: 编译成功（无新警告/错误）。此任务不接线，游戏行为不变。

- [ ] **Step 5: Commit**

```bash
git add src/ConstEnums.h src/Lawn/Travel.h src/Lawn/Travel.cpp
git commit -m "feat(travel): data-driven travel subsystem skeleton (TravelLevelDef/TravelPlantDef tables + queries)"
```

---

### Task 2: 体验关入口 + 关卡参数接线（可从挑战页进局）

**Files:**
- Modify: `src/Lawn/Widget/ChallengeScreen.cpp`（`gChallengeDefs` 表 ~L35-135）
- Modify: `src/Lawn/Board.cpp`（`PickBackground()` ~L1011-1160；`StageHasFog()` ~L9519；`PickZombieWaves()` ~L680；`CanZombieSpawnOnLevel()` ~L2715）
- Modify: `src/Lawn/Board.h`（加成员 `int mTravelFirstFlagSpawned;` 不需要——按下方做法不加；如用"末旗固定补怪"方案才需要。本任务采用下方"白名单 + 不动固定出怪"，无需成员）

**Interfaces:**
- Consumes: `IsTravelLevel`/`GetTravelLevelDef`（Task 1）
- Produces: 挑战页出现可点入口；点击后进入夜间泳池 2 旗自由选卡局（此时还没有新植物与翻页，选卡器只有原版卡）

- [ ] **Step 1: `gChallengeDefs` 加入口行**

在 CRICKET 行（`{ GameMode::GAMEMODE_CHALLENGE_CRICKET, 12, ChallengePage::CHALLENGE_PAGE_CHALLENGE, 4, 0, "斗蛐蛐" }`）**之前**插入：
```cpp
	{ GameMode::GAMEMODE_CHALLENGE_TRAVEL_1,          12,  ChallengePage::CHALLENGE_PAGE_CHALLENGE,   4,  1,  "[TRAVEL_EXPERIENCE]" },
```
（row 4 col 1 是挑战页空位：col 0=斗蛐蛐、col 2=Snowy Day。图标序号 12 与斗蛐蛐相同，如观感不符可换成 0–21 任意值，不影响功能。按钮名走翻译键 `[TRAVEL_EXPERIENCE]`——ChallengeScreen.cpp:491 `TodStringTranslate(aDef.mChallengeName)` 会自动翻译。）

- [ ] **Step 2: 解锁条件——不要被 Trophy 卡住**

阅读 `ChallengeScreen::MoreTrophiesNeeded`（~L270-330）与 `AccomplishmentsNeeded`（~L407）。`GAMEMODE_CHALLENGE_TRAVEL_1` 不应被奖杯数锁定（它是体验关）。检查其默认分支逻辑：若走"挑战页全部按已通关数解锁"的通用分支且默认返回 >0，则在 `MoreTrophiesNeeded` 开头加：
```cpp
	if (aDef.mChallengeMode == GameMode::GAMEMODE_CHALLENGE_TRAVEL_1)
		return 0;
```
（先读函数确认既有分支结构再插入；参照同文件 Snowy Day/CRICKET 的处理方式。CRICKET 若未被锁则说明默认分支对它返回 0，那旅行关同理可复用默认——以读到的实际代码为准，保证**无任何奖杯前置**即可。）

- [ ] **Step 3: 背景 → 夜间泳池**

`Board.cpp` `PickBackground()`（L1011 起的大 switch）：把
```cpp
	case GameMode::GAMEMODE_CHALLENGE_CRICKET:
		mBackground = BackgroundType::BACKGROUND_2_NIGHT;
		break;
```
改为并列加入旅行关（夜间泳池用 `BACKGROUND_4_FOG`，因为 `StageIsNight/StageHasPool/StageHas6Rows` 都以背景判定，FOG = 夜 + 泳池 + 6 行）：
```cpp
	case GameMode::GAMEMODE_CHALLENGE_CRICKET:
		mBackground = BackgroundType::BACKGROUND_2_NIGHT;
		break;

	case GameMode::GAMEMODE_CHALLENGE_TRAVEL_1:
		mBackground = BackgroundType::BACKGROUND_4_FOG;
		break;
```

- [ ] **Step 4: 关掉雾（体验关不要雾）**

`Board::StageHasFog()`（~L9519）末尾返回值加条件：
```cpp
bool Board::StageHasFog()
{
	return !mApp->IsStormyNightLevel() && mApp->mGameMode != GameMode::GAMEMODE_CHALLENGE_INVISIGHOUL && mBackground == BackgroundType::BACKGROUND_4_FOG && !IsTravelLevel(mApp->mGameMode);
}
```
（`Board.cpp` 需 `#include "Travel.h"`；若文件头已含 `LawnCommon.h` 之类，直接追加 include。）

- [ ] **Step 5: 总波数 = 6**

`Board::PickZombieWaves()` 中 mNumWaves 分支（~L680-731，参照 `mApp->IsCricketFightLevel()` 那个 `else if`）：
```cpp
		else if (mApp->IsCricketFightLevel())
			mNumWaves = 1;
```
之后加：
```cpp
		else if (IsTravelLevel(aGameMode))
			mNumWaves = GetTravelLevelDef(aGameMode).mTotalWaves;   // 6
```

- [ ] **Step 6: 教学友好僵尸白名单**

读 `Board::CanZombieSpawnOnLevel(ZombieType theZombieType, int theLevel)`（~L2715，内含按 GameMode/关卡限制可用僵尸的分支）。在其中加旅行关分支（放在函数开头附近便于阅读）：
```cpp
	if (IsTravelLevel(mApp->mGameMode))
	{
		// 教学友好档：普通/路障/铁桶/旗帜 + 少量泳池特色（潜水员、海豚）
		switch (theZombieType)
		{
		case ZombieType::ZOMBIE_NORMAL:
		case ZombieType::ZOMBIE_TRAFFIC_CONE:
		case ZombieType::ZOMBIE_PAIL:
		case ZombieType::ZOMBIE_FLAG:
		case ZombieType::ZOMBIE_SNORKEL:
		case ZombieType::ZOMBIE_DOLPHIN_RIDER:
			return true;
		default:
			return false;
		}
	}
```
（先读该函数确认僵尸类型枚举名一致——`ZombieType` 里潜水员/海豚的确切名字以 `ConstEnums.h` 为准：搜 `SNORKEL`/`DOLPHIN`/`FLAG` 校正名字。若个别类型名不同，用实际名。）

- [ ] **Step 7: 编译并手动验证**

Run: `./build.sh`
Expected: 编译成功。
Run: `./run-pvz.sh`（GUI 人工）→ More Games → Minigames 页 → row 4 / col 1（第 5 行第 2 列，斗蛐蛐右侧的空位）出现可点击入口（文案此时应显示 `<Missing TRAVEL_EXPERIENCE>`——文案在 Task 3 修）；点击 → 正常进入**自由选卡**；选卡后进入**夜间泳池、6 行、无雾**的棋盘；开战 → 波次 6 波、首旗弱、末旗（第 6 波）为旗帜波；僵尸只有普通/路障/铁桶/潜水员/海豚。打到输/赢都正常结算，无崩溃。
（若期间崩溃/卡死：先看是 Step 哪一步引入，单独检查。）

- [ ] **Step 8: Commit**

```bash
git add src/Lawn/Widget/ChallengeScreen.cpp src/Lawn/Board.cpp
git commit -m "feat(travel): travel experience level entry (night pool, 2 flags, 6 waves, tutorial roster)"
```

---

### Task 3: 翻译文件 + 启动加载 + 本地/打包分发

**Files:**
- Create: `properties/pvzp-strings.xml`（仓库顶层新建 `properties/` 目录）
- Create: `properties/pvzp-strings.zh-CN.xml`
- Modify: `src/LawnApp.cpp`（`LoadingThreadProc()` ~L1907-1917）
- Modify: `package.sh`（拷贝段 ~L40-50）
- 本地试玩副本：`/d/workspace/Plants vs. Zombies (AIO v1.0)/Plants vs. Zombies GOTY EN/1.2.0.1073 EN Origin/properties/pvzp-strings.xml`（不入库）

**Interfaces:**
- Consumes: —（独立）
- Produces: 键 `TRAVEL_EXPERIENCE` / `TRAVEL_NEED_FUMESHROOM` / `TRAVEL_PLANT_NAME` / `TRAVEL_PLANT_DESC`（英文默认值；Task 5 会用到后两者）

- [ ] **Step 1: 新建 `properties/pvzp-strings.xml`**

properties XML 语法（参照 `SexyAppFramework/misc/PropertiesParser.cpp` 的 `<String id>` 元素）：
```xml
<!-- PvZ-Portable 旅行模式字符串（英文默认）。中文资源包用户请用 pvzp-strings.zh-CN.xml 内容替换本文件 -->
<PropertySet>
	<String id="TRAVEL_EXPERIENCE">Travel Experience: Puff-shroom Group!</String>
	<String id="TRAVEL_NEED_FUMESHROOM">You need Fume-shroom in your seed pack to upgrade it.</String>
	<String id="TRAVEL_PLANT_NAME">Puff-shroom Group</String>
	<String id="TRAVEL_PLANT_DESC">A Fume-shroom upgraded into a three-headed colony: one big fume head plus two puff-shroom side heads. Each head attacks on its own rhythm.</String>
</PropertySet>
```
（若 `<PropertySet>` 根元素与实际解析不符——先读 `PropertiesParser::ParsePropertiesBuffer` 确认根元素名，用实际根元素。）

- [ ] **Step 2: 新建 `properties/pvzp-strings.zh-CN.xml`**

```xml
<!-- PvZ-Portable 旅行模式字符串（中文备用）。使用中文资源包时，请用本文件内容替换 pvzp-strings.xml -->
<PropertySet>
	<String id="TRAVEL_EXPERIENCE">旅行体验：大喷菇群！</String>
	<String id="TRAVEL_NEED_FUMESHROOM">你的卡组需要包含大喷菇才能升级。</String>
	<String id="TRAVEL_PLANT_NAME">大喷菇群</String>
	<String id="TRAVEL_PLANT_DESC">由大喷菇升级而成的三头菌落：中间一只大喷菇喷烟雾，左右两只小喷菇各自喷孢子，三头独立攻击。</String>
</PropertySet>
```

- [ ] **Step 3: 启动加载（`required=false` 静默）**

`LawnApp.cpp` `LoadingThreadProc()` 中，在
```cpp
	LoadProperties("properties/Layout.xml", false, false);
```
之后加：
```cpp
	LoadProperties("properties/pvzp-strings.xml", false, false);  // 旅行模式自定义字符串（文件缺失时静默）
```

- [ ] **Step 4: 本地副本（保证能直接跑）**

```bash
mkdir -p "/d/workspace/Plants vs. Zombies (AIO v1.0)/Plants vs. Zombies GOTY EN/1.2.0.1073 EN Origin/properties"
cp properties/pvzp-strings.xml "/d/workspace/Plants vs. Zombies (AIO v1.0)/Plants vs. Zombies GOTY EN/1.2.0.1073 EN Origin/properties/pvzp-strings.xml"
```

- [ ] **Step 5: 打包分发**

`package.sh` 在 `cp -R "${PROPERTIES_DIR}" "${STAGING_DIR}/properties"` 之后加：
```bash
# 追加仓库内自定义字符串文件（pak 中不存在该名，FOpen 会 fallback 到磁盘 properties）
if [[ -d "${SCRIPT_DIR}/properties" ]]; then
    cp "${SCRIPT_DIR}/properties/"*.xml "${STAGING_DIR}/properties/"
fi
```

- [ ] **Step 6: 编译 + 手动验证**

Run: `./build.sh`；Run: `./run-pvz.sh` → 挑战页入口现在显示 **"Travel Experience: Puff-shroom Group!"**（不再是 `<Missing ...>`）。

- [ ] **Step 7: Commit**

```bash
git add properties/ src/LawnApp.cpp package.sh
git commit -m "feat(travel): localized strings file (pvzp-strings.xml en/zh) loaded at startup, packaged"
```

---

### Task 4: 选卡器翻页（旅行关专属，页 1 = 旅行植物）

**Files:**
- Modify: `src/Lawn/Widget/SeedChooserScreen.h`（成员/按钮/方法）
- Modify: `src/Lawn/Widget/SeedChooserScreen.cpp`（构造函数、`Draw`、`SeedHitTest`、`GetSeedPositionInChooser`、`ClickedSeedInBank` 相关、`MouseUp`、`Update`、`ShowToolTip` 过滤）
- Modify: `src/LawnApp.cpp`（`HasSeedType` ~L2660 加旅行特判）

**Interfaces:**
- Consumes: `IsTravelOnlySeed`、`IsTravelLevel`（Task 1）
- Produces: `mChooserPage`（0/1）翻页状态；`SeedChooserScreen` 全流程对页 1 种子的支持（本任务结束页 1 仍为空——页 1 内容由 Task 5 注册后出现）

**机制说明（先读再改）：** 选卡器所有"可选卡循环"上限是 `NUM_SEEDS_IN_CHOOSER`(=49，即 0..SEED_IMITATER)。旅行植物枚举值 >49，需：① 这些循环上限放开到 `NUM_SEED_TYPES`（安全：49..NUM_SEED_TYPES-1 之间其它未拥有植物会被 `mApp->HasSeedType()` 过滤掉）；② `HasSeedType(SEED_FUMESHROOM_GROUP)` 在旅行关返回 true（普通关 false）；③ 页 0 显示时跳过旅行专属种子、页 1 显示时只显示旅行专属种子；④ 页 1 种子的 `GetSeedPositionInChooser` 坐标按其"页内索引"排布。

- [ ] **Step 1: 读现状（必须）**

读 `SeedChooserScreen.cpp` 构造函数（~L100-224，注意 `memset(mChosenSeeds,0,...)` 后仅初始化 0..NUM_SEEDS_IN_CHOOSER-1；`SEED_IN_CHOOSER==3`，未初始化条目 state==0），`Draw`（~L344-441），`SeedHitTest`（~L774），`FindSeedInBank`（~L786），`ClickedSeedInChooser`（~L840），`ClickedSeedInBank`（~L808），`ShowToolTip`（~L861-948），`MouseUp`（~L960）。

- [ ] **Step 2: `LawnApp::HasSeedType` 旅行特判**

`LawnApp.cpp` `HasSeedType`（~L2660）switch 的 `default:` 前加：
```cpp
	case SeedType::SEED_FUMESHROOM_GROUP:
		return IsTravelLevel(mGameMode);   // 旅行关专属：仅旅行关可拥有/可选
```
（`LawnApp.cpp` 需 `#include "Lawn/Travel.h"`。同时确认 `LawnApp::GetSeedsAvailable()` 不影响——它用于解锁判定，保持原样即可。）

- [ ] **Step 3: `SeedChooserScreen.h` 增加翻页状态与按钮**

```cpp
    int                     mChooserPage;          // 0 = 原版卡页；1 = 旅行植物页
    GameButton*             mChooserPageButton;    // 页 1 切换按钮（仅旅行关显示）
```
（加到 public 成员区，仿 `mImitaterButton` 声明位置。）

- [ ] **Step 4: 构造函数初始化 + 建按钮**

构造函数中（在 `mImitaterButton` 等按钮创建区附近）：
```cpp
	mChooserPage = 0;
	mChooserPageButton = MakeNewButton(SeedChooserScreen::SeedChooserScreen_Page2, this, "»", nullptr, Sexy::IMAGE_SEEDCHOOSER_BUTTON2,
		Sexy::IMAGE_SEEDCHOOSER_BUTTON2_GLOW, Sexy::IMAGE_SEEDCHOOSER_BUTTON2_GLOW);
	mChooserPageButton->Resize(650, 540, 40, 40);
	mChooserPageButton->mVisible = IsTravelLevel(mApp->mGameMode);
	mChooserPageButton->mBtnNoDraw = !IsTravelLevel(mApp->mGameMode);
```
（按钮 id 枚举 `SeedChooserScreen_Page2 = 107` 加进文件头部 private enum；图标用现有 `IMAGE_SEEDCHOOSER_BUTTON2` 资源。若 MakeNewButton 需资源名/颜色照抄 `mBackButton` 创建写法——先读构造函数内 `mStartButton` 等的真实创建方式，仿写。若 `mChooserPageButton` 文字想用翻译键 `[TRAVEL_NEXT_PAGE]` 则 Task 3 文件补键；本任务先用字面 "»"/"«" 即可，视觉细节后续可调。）

页 1 种子坐标初始化：构造函数 `for (SeedType aSeedType = SEED_PEASHOOTER; ... < NUM_SEED_TYPES; ...)` 扩展现有初始化循环？**不要**扩展现有循环（会改动 0..48 语义）。在构造函数末尾追加：
```cpp
	// 旅行专属种子初始化（页 1 网格，默认 SEED_IN_CHOOSER）
	for (int i = 0; i < NUM_TRAVEL_PLANTS; i++)
	{
		SeedType aTravelSeed = gTravelPlantDefs[i].mSeedType;
		ChosenSeed& aChosenSeed = mChosenSeeds[aTravelSeed];
		aChosenSeed.mSeedType = aTravelSeed;
		GetSeedPositionInChooser(aTravelSeed, aChosenSeed.mX, aChosenSeed.mY);  // Step 5 会把它映射到页内索引
		aChosenSeed.mTimeStartMotion = 0;
		aChosenSeed.mTimeEndMotion = 0;
		aChosenSeed.mStartX = aChosenSeed.mX;
		aChosenSeed.mStartY = aChosenSeed.mY;
		aChosenSeed.mEndX = aChosenSeed.mX;
		aChosenSeed.mEndY = aChosenSeed.mY;
		aChosenSeed.mSeedState = SEED_IN_CHOOSER;
		aChosenSeed.mSeedIndexInBank = 0;
		aChosenSeed.mRefreshCounter = 0;
		aChosenSeed.mRefreshing = false;
		aChosenSeed.mCrazyDavePicked = false;
		aChosenSeed.mImitaterType = SeedType::SEED_NONE;
	}
```

- [ ] **Step 5: `GetSeedPositionInChooser` 页 1 布局**

`SeedChooserScreen::GetSeedPositionInChooser(int theIndex, int& x, int& y)`（~L283）开头（imitater 特判之前）加：旅行专属种子按其**在 `gTravelPlantDefs` 中的序号**（页内索引 0..）布局，与页 0 同公式：
```cpp
	if (IsTravelOnlySeed((SeedType)theIndex))
	{
		int aPage1Index = 0;
		for (int i = 0; i < NUM_TRAVEL_PLANTS; i++)
			if (gTravelPlantDefs[i].mSeedType == theIndex)
				aPage1Index = i;
		int aRow = aPage1Index / 8;
		int aCol = aPage1Index % 8;
		x = aCol * 53 + 22;
		y = aRow * 70 + 123;
		return;
	}
```

- [ ] **Step 6: 循环上限放开 + 页面过滤**

凡遍历可选卡且含 `HasSeedType` 过滤的循环，把上限 `NUM_SEEDS_IN_CHOOSER` 改为 `NUM_SEED_TYPES`，并在循环内过滤：
```cpp
	if (IsTravelOnlySeed(aSeedType) && mChooserPage == 0) continue;   // 页 0 不显示旅行植物
	if (!IsTravelOnlySeed(aSeedType) && mChooserPage == 1) continue;   // 页 1 只显示旅行植物
```
需要放开上限的位置（逐个核对：当前上限是 `NUM_SEEDS_IN_CHOOSER` 的循环/遍历）：
- `Draw()`：灰剪影循环（~L361，上限用 `aNumSeeds` 保持不变——页 0 网格剪影）；已选卡绘制循环（~L398）；飞行卡绘制循环（~L422）
- `Update()` 飞行插值循环（~L536）
- `SeedHitTest`（~L774）
- `FindSeedInBank`（~L786）
- `MouseUp`（~L960-970 内对 SEED_IN_CHOOSER 的点击判定循环）
- `ShowToolTip`（~L888 的 tooltip 判定）
- `CrazyDavePickSeeds`（~L224-258，随机选卡）：**不要**放开（页 1 卡不参与随机），保持原样；若其循环含 `IsUpgrade` 过滤则旅行卡天然被排除。

注意 `Draw()` 的"已选卡"绘制循环原来画 SEED_IN_BANK 卡在卡槽位置——旅行卡同样适用（其 x/y 落位后就是卡槽坐标，无需特判）。

- [ ] **Step 7: `ClickedSeedInChooser` 落位后画卡**

`ClickedSeedInChooser`（~L840）用 `mSeedsInBank` 递增并 `GetSeedPositionInBank`——对旅行卡同样成立（只需其初始 pos 正确，Step 4/5 已保证）。**不需要**改。

- [ ] **Step 8: `MouseUp`/`ClickedSeedInBank` 返回路径**

`ClickedSeedInBank`（~L808）把卡送回 `GetSeedPositionInChooser(theChosenSeed.mSeedType)`——旅行卡会走 Step 5 映射回页 1 网格 ✓ 无需改。`MouseUp` 中点选卡槽逻辑（若按 `FindSeedInBank` 命中后调 `ClickedSeedInBank`）自动成立。

- [ ] **Step 9: 翻页按钮回调**

`SeedChooserScreen::ButtonDepress`（~L730 区域，含 `SeedChooserScreen_Imitater` case）加：
```cpp
	else if (theId == SeedChooserScreen::SeedChooserScreen_Page2)
	{
		mChooserPage = mChooserPage == 0 ? 1 : 0;
		mApp->PlaySample(Sexy::SOUND_TAP);
	}
```
（先读 ButtonDepress 的 id 分发写法，按实际结构插入；若回调经 `Widget::ButtonDepress`/`ButtonPress` 分发的另一个函数，改在那个函数。确认 `mChooserPageButton->mVisible` 已按 IsTravelLevel 设定；普通模式按钮不存在。）

- [ ] **Step 10: `ShowToolTip` 兜底**

`ShowToolTip`（~L861-948）对悬停卡取名称/描述。旅行卡此时若取不到图鉴文本会显示 `<Missing ...>` 或空——可接受（不崩溃），但**确保不会越界/断言**：读该函数，确认它对任意 SeedType 都安全（如 `Plant::GetNameString` 或图鉴键缺失时不 assert）。若对未知种子会 assert/崩溃，则对 `IsTravelOnlySeed` 分支直接返回不显示 tooltip。

- [ ] **Step 11: 编译 + 手动验证（页 1 暂空）**

Run: `./build.sh`；Run: `./run-pvz.sh` → 旅行体验关选卡界面：页 0 与原版一致；点击 "»" 按钮翻到页 1（此时页 1 空白/只有背景——Task 5 注册后才有卡）；再点回页 0；**其它模式（冒险/生存/普通小游戏）选卡界面不出现该按钮**；无崩溃。

- [ ] **Step 12: Commit**

```bash
git add src/Lawn/Widget/SeedChooserScreen.h src/Lawn/Widget/SeedChooserScreen.cpp src/LawnApp.cpp
git commit -m "feat(travel): seed chooser paging UI (travel-only page, chooser loops extended to NUM_SEED_TYPES)"
```

---

### Task 5: 大喷菇群注册与升级机制（页 1 出现卡片并可升级）

**Files:**
- Modify: `src/Lawn/Plant.cpp`（`gPlantDefs` ~L96-99 尾；`Plant::IsUpgrade` ~L5550；`Plant::IsUpgradableTo` ~L2867-2900；`Plant::IsNocturnal` ~L5505-5530；`Plant::GetNameString` ~L5461）
- Modify: `src/Lawn/Plant.h`（如 GetNameString/GetRefreshTime 需扩展签名——先读再定）
- Modify: `src/Lawn/SeedPacket.cpp`（`DrawSeedPacket` 缩放 case ~L359）
- Modify: `src/Lawn/Widget/SeedChooserScreen.cpp`（`OnStartButton` 同选校验链 ~L695-703）
- 若 `Plant::GetRefreshTime`/其它按 SeedType 查 `mRefreshTime` 的路径已通用（基于 PlantDefinition），无需改。

**Interfaces:**
- Consumes: `SEED_FUMESHROOM_GROUP` 枚举、`HasSeedType` 旅行特判、翻页 UI（Task 4）
- Produces: 页 1 出现"大喷菇群"紫卡（0 阳光）；可选中；选了大喷菇群未选大喷菇时点开始弹同选警告；场上种大喷菇后拖紫卡上去 → 替换成三头形态（渲染在 Task 6）；铲掉/被吃整格消失

- [ ] **Step 1: `gPlantDefs` 注册**

`Plant.cpp` `gPlantDefs[]` 末尾（`SEED_LEFTPEATER` 行后）加：
```cpp
	{ SeedType::SEED_FUMESHROOM_GROUP,  nullptr, ReanimationType::REANIM_FUMESHROOM, 9, 0, 3000, PlantSubClass::SUBCLASS_SHOOTER, 90, "FUMESHROOM_GROUP" }
```
（字段序：mSeedType, mPlantImage, mReanimationType, mPacketIndex=9(与大喷菇同卡图资源), mSeedCost=0, mRefreshTime=3000(30s 紫卡冷却), mSubClass=SHOOTER, mLaunchRate=90(中间头攻击间隔,与大喷菇一致), mPlantName。）
若该数组前一行（LEFTPEATER）行尾无逗号，注意补逗号。

- [ ] **Step 2: `Plant::IsUpgrade` 加类型**

`Plant.cpp` `Plant::IsUpgrade`（~L5550）返回列表加：
```cpp
		theSeedtype == SeedType::SEED_FUMESHROOM_GROUP ||
```

- [ ] **Step 3: `Plant::IsUpgradableTo` 加规则**

`Plant.cpp` `Plant::IsUpgradableTo`（~L2867）在 GLOOMSHROOM 那条后加：
```cpp
	if (theUpgradedType == SeedType::SEED_FUMESHROOM_GROUP && mSeedType == SeedType::SEED_FUMESHROOM)
	{
		return true;
	}
```

- [ ] **Step 4: `Plant::IsNocturnal` 加类型**

`Plant.cpp` `Plant::IsNocturnal`（~L5505-5530 列表）加 `SEED_FUMESHROOM_GROUP`（大喷菇群是蘑菇，白天会睡——本关夜间无影响，但保证正确性）。

- [ ] **Step 5: `Plant::GetNameString` 支持**

读 `Plant::GetNameString`（~L5461）实现。它很可能从图鉴/属性取名称或按类型映射。保证 `GetNameString(SEED_FUMESHROOM_GROUP)` 返回可读名（用于同选警告的 `{UPGRADE_TO}`）。若函数按 SeedType switch 返回：加
```cpp
	case SeedType::SEED_FUMESHROOM_GROUP:
		return TodStringTranslate("[TRAVEL_PLANT_NAME]");   // "Puff-shroom Group" / 中文 "大喷菇群"
```
若函数走 `gPlantDefs[].mPlantName` 查字典则无需改（Task 3 文件里给 `[TRAVEL_PLANT_NAME]` 键，本任务确保有映射途径即可；若 mPlantName "FUMESHROOM_GROUP" 直接当文本显示，则在 GetNameString 内为该类型特判返回翻译键）。

- [ ] **Step 6: 卡片绘制缩放**

`SeedPacket.cpp` `DrawSeedPacket` 中与 `SEED_FUMESHROOM` 同组的缩放 case（~L359）加：
```cpp
	case SeedType::SEED_FUMESHROOM_GROUP:
```
（与大喷菇同样 0.4 缩放绘制 reanim 缩略图；紫卡背景由 `Plant::IsUpgrade` 自动变 cel 1——本文件 ~L297 已按 IsUpgrade 选背景，无需再改。）

- [ ] **Step 7: 同选校验**

`SeedChooserScreen.cpp` `OnStartButton` 的升级校验链（~L695-703，`CheckSeedUpgrade(GLOOMSHROOM, FUMESHROOM)` 那行后）加：
```cpp
		!CheckSeedUpgrade(SEED_FUMESHROOM_GROUP, SEED_FUMESHROOM) ||
```
（`CheckSeedUpgrade` 通用实现已存在：未选基础植物时弹 `[SEED_CHOOSER_UPGRADE_WARNING]` 并把 `{UPGRADE_TO}` 替换为 `GetNameString(GROUP)`——Step 5 保证名称可读。弹窗按钮文案用既有键，无需新键。）

- [ ] **Step 8: 种植/升级路径核对（预计零改动，但验证）**

- `Board::CanPlantAt`（~L3300-3375）：普通路径已支持任意 `IsUpgrade` 种子（PLANTING_NEEDS_UPGRADE）与 `IsUpgradableTo` 目标格（PLANTING_OK）→ 无需改。
- 种植替换（~L4375 附近，`Board::Plant` 或 `AddPlant`）：`aNormalPlant->IsUpgradableTo(aPlantingSeedType)` 为真时 `Die()` 后种新植物 → 升级自动成立（血量=新植物满血 ✓ 铲除=Die 消失 ✓）。
- 确认替换处对 `SEED_GLOOMSHROOM` 的 `aIsAwake` 特判是 gloom 专属，GROUP 不需要（夜间关）。

- [ ] **Step 9: 编译 + 手动验证**

Run: `./build.sh`；Run: `./run-pvz.sh`：
1. 旅行体验关选卡器翻到页 1 → 出现"大喷菇群"紫卡（紫色背景、0 阳光、30s 冷却圆盘）→ 可点选，飞入卡槽
2. 只选紫卡不选大喷菇 → 点开始 → 弹"需 Fume-shroom"警告（`{UPGRADE_TO}` = "Puff-shroom Group"）；补选大喷菇后可开始
3. 开局种大喷菇 → 拖紫卡到已种大喷菇格 → 放行、阳光不扣（0 阳光）→ 该格替换（此时渲染仍是单头——Task 6 做成三头）
4. 拖紫卡到空地/其它植物 → 红禁
5. 铲掉升级后的植物 → 整格消失
6. 普通模式选卡器无紫卡、无翻页（回归）

- [ ] **Step 10: Commit**

```bash
git add src/Lawn/Plant.cpp src/Lawn/Plant.h src/Lawn/SeedPacket.cpp src/Lawn/Widget/SeedChooserScreen.cpp
git commit -m "feat(travel): puff-shroom group upgrade card (0 sun, 30s cd, upgrades fume-shroom, same-pack check)"
```

---

### Task 6: 三头渲染 + 各喷各的攻击行为

**Files:**
- Modify: `src/Lawn/Plant.h`（新成员）
- Modify: `src/Lawn/Plant.cpp`（初始化/渲染/攻击分支）
- Modify: `src/Lawn/Board.cpp`（若攻击判定需要 Board 帮助——先读再定）
- Modify: `src/Lawn/Projectile.cpp`（孢子斜飞/射程若需新弹道参数——先读再定）

**Interfaces:**
- Consumes: `SEED_FUMESHROOM_GROUP` 完整注册（Task 5）
- Produces: 升级后该格渲染 = 中间正常大喷菇 + 左右缩小版小喷菇；攻击 = 中间按大喷菇节奏喷 3×3 穿透烟雾（本行）；两侧按小喷菇节奏喷单体孢子（PROJECTILE_PUFF），微微斜飞（左斜上/右斜下）、**不锁行**、撞第一个僵尸消失，飞行距离上限 = 大喷菇烟雾射程；三头独立计时。

**关键参照（先读这些代码再动手）：**
- 大喷菇攻击管道：`Plant.cpp` 中 `mShootingCounter` 相关（~L819-820 设置攻击前摇：FUMESHROOM=50/PUFFSHROOM=29；~L3526 fume 攻击触发时刻；~L560-740 `DoRowAreaDamage`/`GetPlantAttackRect` 区域伤害实现——fume 的 3×3 持续伤害按"攻击矩形内每 tick 伤害"实现，读 560-750 确认调用方式）。
- 扇形/角度弹道先例：`Plant.cpp` 中 gatling 大招散射段（搜 `mVelX = PEA_SPEED * cos`，~L700-760 附近），它创建 Projectile 后设 `mMotionType = MOTION_STAR` + `mVelX/mVelY`（斜向速度）——这就是"不锁行斜飞"。
- 弹丸位置更新与命中：`Projectile.cpp` `mPosX += mVelX`（~L653）与命中判定（~L208-280 区域按弹体矩形对僵尸迭代，不限行）。
- 孢子射程限制：读 `Projectile.cpp` 中 `MOTION_STAR` 的销毁条件（~L540）与 puff 弹既有射程逻辑，选择限制手段（比如设 `mDamageRange`/飞行距离计数或在到达大喷菇射程时 `Die()`）。

- [ ] **Step 1: 读透现有攻击调度**

读 `Plant.cpp` 攻击主调度（找 `mShootingCounter--` 的 Update 路径与 FUMESHROOM/PUFFSHROOM 各自"开火时刻"代码，确认：大喷菇在某个 counter 值向 3×3 区域施放伤害/粒子；小喷菇在同一调度里 `AddProjectile(PROJECTILE_PUFF)`）。同时读 `Plant.h` L160-210 的 reanim/状态成员，确认现有多 reanim 机制（`mHeadReanimID2/3` 被哪些植物使用、如何创建/更新/销毁）。

- [ ] **Step 2: `Plant.h` 加成员**

```cpp
	// 大喷菇群（SEED_FUMESHROOM_GROUP）：两侧小喷菇子动画与独立攻击计时
	ReanimationID           mPuffLReanimID;
	ReanimationID           mPuffRReanimID;
	int                     mPuffLShootCounter;    // 左头攻击前摇计数（Puff 节奏）
	int                     mPuffRShootCounter;    // 右头攻击前摇计数
```
（成员命名风格对齐现有 `mHeadReanimID` 等；若实现中发现可直接复用 `mHeadReanimID2/3` 作左右头，则不加新 reanim 成员，改用现成字段——以读到的实际代码决定，取最小改动。）

- [ ] **Step 3: 初始化（升级/种下时）**

在 Plant 的创建初始化路径（找现有 `mBodyReanimID` 创建处，比如 `PlantInitialize`/`UpdateReanim` 内按 mReanimationType AddReanimation 的地方）对 `SEED_FUMESHROOM_GROUP`：body 用 `REANIM_FUMESHROOM`（正常比例）；另建两个 `REANIM_PUFFSHROOM` 子动画（左右缩小，`OverrideScale` 约 0.6 或按比例），x 偏移左右各约 ±22px、y 偏移约 +10px（对齐脚底），render order 在 body 之后（参考现有多 reanim 渲染/同步位置的方法：多数植物在 `Plant::Draw` 或 `UpdateReanim` 里把子 reanim `SetPosition(mX+off, mY+off)` 跟随）。升级路径（Task 5 Step 8 的替换）会整格重建 Plant → 初始化代码自动生效。
左右头初始攻击前摇错开：`mPuffLShootCounter = 10; mPuffRShootCounter = 20;`（相位错开，观感自然）。
销毁：`Plant::Die()`/析构处确保子 reanim 一并移除（找现有 `ReanimationDie`/`ReanimationTryToGet` 清理模式，把两个新 id 加进清理清单）。

- [ ] **Step 4: 渲染跟随**

在 Plant 渲染/位置同步处（找 body reanim 每帧 `SetPosition` 的公共函数，可能在 `Plant::Draw` 或专门的 reanim sync），让左右小喷菇 reanim 跟随本格坐标（x ± 22px 偏移、与 body 同 y 基准；若为深度排序需要微调 mRenderOrder 用 `ReanimationSetRenderOrder`/AddReanimation 的 renderOrder 参数）。

- [ ] **Step 5: 攻击调度分支**

在攻击调度函数里找到 FUMESHROOM/GLOOMSHROOM/PUFFSHROOM 的 case 区（含 ~L819-820 counter 初值处），为 `SEED_FUMESHROOM_GROUP` 加独立分支：
- 中间头：复用大喷菇的整套判定（3×3 区域烟雾伤害 + 攻击动画），节奏取 mLaunchRate=90 对应值（若原代码按 mShootingCounter 前摇 50 后对攻击矩形内僵尸做伤害，直接复用同一段，只是 `mSeedType == FUMESHROOM` 的判断扩为 `== FUMESHROOM || == FUMESHROOM_GROUP` 或在新分支内调用同函数——以读到的结构选择，**不要**改动普通大喷菇行为）。
- 左右头（各喷各的）：各自维护 `mPuffLShootCounter`/`mPuffRShootCounter`，倒计时到 0 时：朝前发射 `PROJECTILE_PUFF`（`mBoard->AddProjectile(...)`），起点 x/y 分别为本格左侧/右侧小喷菇喷口（x ± 约 10-15px、y 同小喷菇头部高度），设角度速度（左头 vy 微负=斜上，右头 vy 微正=斜下；vx 与普通孢子一致水平速度、vy ≈ vx * ±0.08 之类小角度）；然后 counter 重置为 Puff 节奏（参考 PUFFSHROOM 的间隔，用其 mLaunchRate=75 换算或照抄 puff 的实际间隔值——以读到的 puff 调度为准）。射程限制：给左右孢子设置水平飞行距离上限 = 大喷菇烟雾射程（先读 fume 的攻击矩形宽/射程值），实现为 projectile 自身属性（如加"最大水平距离，超出 Die()"的计数）或复用 PROJECTILE_PUFF 既有距离机制把上限调大——以 Projectile.cpp 读到的机制选最小实现；若 PROJECTILE_PUFF 已有"到点消失"逻辑则直接设对参数。
- 睡眠/唤醒：若夜间不睡，检查现有 `IsNocturnal`/`IsFungus` 相关睡眠逻辑是否对 GROUP 有影响（夜间关无事，但确认白天种植时表现一致——Task 5 已把 GROUP 加入 IsNocturnal）。

- [ ] **Step 6: 编译 + 手动验证**

Run: `./build.sh`；Run: `./run-pvz.sh`：
1. 升级大喷菇 → 该格出现**三头**：中间大喷菇 + 左右两只小号小喷菇（视觉正常、不重叠错乱、被吃/铲掉无残留 reanim 报错）
2. 观察攻击：中间按大喷菇节奏喷烟雾；左右小喷菇**各自独立节奏**喷孢子（视觉上三头错落开火）
3. 孢子：单体——撞到第一个僵尸消失并出 puff 溅射粒子；斜飞——左头微上、右头微下，能命中邻行边缘的僵尸；射程——与大喷菇烟雾射程一致，到点消散不横穿全屏
4. 中间烟雾仍是本行 3×3 穿透
5. 回归：普通大喷菇/小喷菇行为不变；升级其它紫卡（忧郁菇等）不受影响

- [ ] **Step 7: Commit**

```bash
git add src/Lawn/Plant.h src/Lawn/Plant.cpp src/Lawn/Board.cpp src/Lawn/Projectile.cpp
git commit -m "feat(travel): puff-shroom group three-head render & independent attacks (fume + two angled single-hit spores)"
```

---

### Task 7: 端到端手动验证 + 收尾

**Files:**
- 可能微调：`CONTEXT.md`（领域词典追加旅行模式小节——可选但推荐，参照既有小节格式）
- 无其它代码改动（除非发现缺陷）

- [ ] **Step 1: 完整手动回归清单（GUI，逐项打勾）**

用 `./run-pvz.sh` 逐项验证 spec 第 6 节清单：
1. 挑战页出现新入口"Travel Experience: Puff-shroom Group!"；其它页/关卡无变化
2. 旅行关选卡器有翻页按钮，页 1 有"大喷菇群"紫卡；普通关无翻页按钮
3. 只选紫卡不选大喷菇 → 开始被拦 + 警告；补选后可开始
4. 种大喷菇 → 拖紫卡升级 → 三头外观、三头独立节奏
5. 中间烟雾穿透本行；两侧孢子单体、斜飞、射程与大喷菇一致、命中消散/到点消散
6. 铲掉或僵尸吃掉大喷菇群 → 整格消失
7. 夜间泳池 6 波 / 2 旗帜（第 1、6 波）：首旗弱教学、末旗铁桶+潜水员/海豚；通关结算正常、可重试
8. 回归：冒险/生存/其它小游戏选卡器、图鉴、模仿者、咖啡豆等均正常；普通大喷菇与其它紫卡升级不受影响
9. 文案：英文资源包显示英文标题；把 `properties/pvzp-strings.xml` 换成 zh-CN 内容后显示"旅行体验：大喷菇群！"

- [ ] **Step 2: 记录并修复发现的问题**

发现问题 → 用 systematic-debugging 流程修复 → 重新验证 → 单独提交（`fix(travel): ...`）。

- [ ] **Step 3: 文档（可选推荐）**

`CONTEXT.md` 增加"旅行模式"小节：术语（旅行关卡 TravelLevel、旅行专属植物 Travel Plant、页/翻页）与关系（旅行专属植物只在旅行关卡选卡器页 1 出现；升级卡大喷菇群 = 紫卡 + 30s 冷却 + 只能升级大喷菇）。

- [ ] **Step 4: 最终提交**

```bash
git add CONTEXT.md
git commit -m "docs(travel): CONTEXT.md glossary for travel mode subsystem"
```

---

## 自审备注（编写时已核对）

- **Spec 覆盖**：子系统骨架(T1) / 入口与关卡(T2) / 文案(T3) / 翻页(T4) / 紫卡与升级(T5) / 三头攻击(T6) / 测试(T7) 一一对应；夜间泳池 6 波 2 旗、自由选卡、教学白名单、无解锁链、零新素材均已落任务。
- **存档兼容**：所有枚举前插；挑战模式无中场存档；`mTravelId` 预留。
- **本计划的关键开放点已收敛为"先读后改"的明确锚点**（Task 4 Step 1、Task 5 Step 5/8、Task 6 Step 1）——实现者必须按锚点阅读再落笔，行号可能因 HEAD 漂移，以函数名为准。
