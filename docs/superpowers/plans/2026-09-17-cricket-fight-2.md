# 斗蛐蛐 2（录制沙盒）实现计划

**Goal:** 新增小游戏「斗蛐蛐 2」：5 行草坪录制沙盒 —— 自选僵尸种类 × 自选出怪倍率、坚不可摧式「开始战斗」按钮、无限阳光、完全不判负。

**Spec:** `docs/superpowers/specs/2026-09-17-cricket-fight-2-design.md`

**硬性约束**

- 不重排 `ConstEnums.h` 既有枚举值；新值插在 `NUM_GAME_MODES` 哨兵之前
- 不为植物/僵尸建子类；沿用 `SeedType` / `ZombieType` 分支风格
- 新文案优先复用 pak 既有键（`[START_ONSLAUGHT]`）或字面量中文（项目既有先例：`"斗蛐蛐"`、`"背包"`），避免用户 properties 缺失时显示 `<Missing>`
- 每个 `.h`/`.cpp` 保持既有 LGPL 头

---

## Task 1 — GameMode 与 LawnApp 帮助函数

- Modify: `src/ConstEnums.h`（`GAMEMODE_CHALLENGE_TRAVEL_JOURNEY` 之后）

```cpp
    GAMEMODE_CHALLENGE_TRAVEL_JOURNEY,               // 旅行模式：11 轮完整路线
    GAMEMODE_CHALLENGE_CRICKET_2,                    // 斗蛐蛐 2：录制沙盒（自选僵尸出怪 + 倍率 + 无限阳光）
    GAMEMODE_UPSELL,
```

- Modify: `src/LawnApp.h`（`IsCricketFightLevel()` 声明旁）

```cpp
	/*inline*/ bool					IsCricketFight2Level();
```

- Modify: `src/LawnApp.cpp`（`IsCricketFightLevel()` 定义之后）

```cpp
bool LawnApp::IsCricketFight2Level()
{
	return mGameMode == GameMode::GAMEMODE_CHALLENGE_CRICKET_2;
}
```

- Modify: `src/LawnApp.cpp` `HasSeedType()` 开头：本模式放行全部常规植物（`SEED_PEASHOOTER`..`SEED_IMITATER`），旅行专属卡仍不开放。

- Verify: `rg "GAMEMODE_CHALLENGE_CRICKET_2|IsCricketFight2Level" src`

---

## Task 2 — 挑战页入口

- Modify: `src/Lawn/Widget/ChallengeScreen.cpp` `gChallengeDefs` 末尾（`GAMEMODE_INTRO` 之前）

```cpp
	{ GameMode::GAMEMODE_CHALLENGE_CRICKET_2,                  12,  ChallengePage::CHALLENGE_PAGE_CHALLENGE,   5,  0,  "斗蛐蛐2" },
```

（挑战页 5×5 已满 → 用第 6 行；`UpdateButtons()` 已按 `mScrollY` 滚动，`aContentBottom` 由最大行号算出，滚轮可进入视野。）

- Modify: `MoreTrophiesNeeded()` 开头的永久可用列表，加入 `aDef.mChallengeMode == GameMode::GAMEMODE_CHALLENGE_CRICKET_2`

- Verify: `rg "斗蛐蛐2" src`

---

## Task 3 — Board 状态与关卡初始化

- Modify: `src/Lawn/Board.h`：`IsIceSandboxLevel()` 声明附近加入 `IsCricket2Level()` / 出怪面板 / 战斗流程方法声明与状态成员
- Modify: `src/Lawn/Board.cpp` 构造函数：初始化 `mCricket2Prep = true;`、`mCricket2ZombieMultiplier = 1;`、`mCricket2PanelOpen = false;`、`mCricket2ButtonShownPrep = -1;`、`memset(mCricket2ZombieEnabled, 0, ...)` 后仅置 `ZOMBIE_NORMAL = true`
- Modify: `src/Lawn/Board.cpp` 构造函数：`mStoreButton` 创建条件加入本模式
- Modify: 以下函数各加一行本模式分支
  - `PickBackground` → `BACKGROUND_2_NIGHT`
  - `PickZombieWaves` → `mNumWaves = 1`，并把 `mZombiesInWave[0][0] = ZOMBIE_INVALID`
  - `InitLevel` → `mSunMoney = 9990`；重置出怪面板状态
  - `InitLawnMowers` → 早退（无小推车）
  - `GetNumSeedsInBank` → `return 10;`
  - `HasProgressMeter` → `return false;`
  - `AddSunMoney` → `theAmount < 0` 时直接 return
  - `AddCoin` → `COIN_SUN` 直接 return nullptr
  - `UpdateSunSpawning` → 早退

- Verify: 编译通过；进关后阳光 9990、种植不掉、无小推车、无进度条

---

## Task 4 — 出怪设置面板 UI

- Modify: `src/Lawn/Board.cpp` 冰冻沙盒区之后：新增「斗蛐蛐 2 · 出怪设置」面板
  - 复用匿名命名空间里的 `gIceSandboxZombieTypes`（20 种安全僵尸）与 `IceSandboxZombieCardName()`
  - 面板 `Rect(60, 20, 680, 560)`；7 列 × 3 行卡网格；卡面底色/边框按开关注得区分，附实心/空心指示方块
  - 倍率行：`[-10] [-1] ×N [+1] [+10]`，范围 1–50
  - 汇总行、`全选` / `全不选` / `关闭` 按钮、底部提示
- Modify: `Board::MouseDown`：面板打开时优先交给 `CricketFight2PanelMouseDown`
- Modify: `Board::MouseDown`：未开面板时左下「出怪设置」按钮切换面板
- Modify: `Board::KeyDown`：`Esc` 关面板
- Modify: `DrawUIBottom`（或 `Board::Draw` 末尾）：调用 `DrawCricket2UI(g)`

- Verify: 手动进关点开面板，勾选/倍率/汇总数字正确联动

---

## Task 5 — 开始战斗按钮与战斗流程

- Modify: `src/Lawn/Challenge.cpp` `UpdateZombieSpawning()` 返回值加入本模式（恒 `true`：完全接管出怪，波次系统不参与）
- Modify: `src/Lawn/Board.cpp`

```cpp
Rect Board::GetCricket2StartButtonRect();  // Rect(300, 546, 210, 46)，与坚不可摧同位
void Board::CricketFight2StartBattle();    // 轮转枚举已选种类 × 倍率 → AddZombie(type, ZOMBIE_WAVE_DEBUG)，截断 300
void Board::CricketFight2EndBattle();      // 清僵尸/子弹/粒子/金币 → mCricket2Prep = true
```

- Modify: `Board::MouseDown` → `CricketFight2HandleMouseDown` 分派开始/结束（**在 MouseDown 里消费点击**，避免像坚不可摧那样在 MouseUp 触发而误种植物）
- Modify: `Board::Draw` 末尾 → `DrawCricket2UI(g)`（用 `DrawStoneButton()` 画左下「出怪设置」与底部中央「开始战斗/结束战斗」）
- 不创建/不复用 `mStoreButton`（本模式它保持 nullptr，`DrawUITop`/`DrawUIBottom`/构造函数都不用改）

- Verify: 点开始 → 僵尸一次性登场；点结束 → 清场且植物保留；不点开始永远不出怪

---

## Task 6 — 纯沙盒（不判负 / 不通关 / 不要墓碑）

- Modify: `src/Lawn/Zombie.cpp` `CheckForBoardEdge()`

```cpp
if (mApp->IsIZombieLevel() || mBoard->IsIceSandboxLevel() || mApp->IsCricketFight2Level())
```

- Modify: `src/Lawn/Zombie.cpp` `TrySpawnLevelAward()` 开头

```cpp
if (mApp->IsCricketFight2Level())
    return false;   // 斗蛐蛐 2：纯沙盒，没有胜利结算
```

- Modify: `src/Lawn/Board.cpp` `StageHasGraveStones()` 排除列表加入本模式（夜战草坪默认长墓碑，会挡住布阵）

- Verify: 僵尸走到最左安静消失；僵尸全灭后无奖杯币、无结算弹窗、不退出关卡；草坪上没有墓碑

---

## Task 7 — 构建与人工验证

Run: `cmake --build build`（Release，已有 `build/` 目录）

人工清单见 spec「验证」一节。重点回归：
- 冒险模式、斗蛐蛐 1、坚不可摧的开始战斗按钮行为不变
- 冰冻沙盒背包仍正常（本模式新增的鼠标分支不得抢在冰冻沙盒之前）
- 非本模式 `Esc` / `Z` 等按键行为不变

---

## 类型一致性检查

- `IsCricketFight2Level()`、`IsCricket2Level()`、`CricketFight2StartBattle()`、`CricketFight2EndBattle()`、`CricketFight2Update()`、`CricketFight2HandleMouseDown(int,int,int)`、`GetCricket2PanelButtonRect()` 定义与调用处签名一致
- `mCricket2ZombieEnabled[]` 下标类型为 `ZombieType`，数组长度 `ZombieType::NUM_ZOMBIE_TYPES`
- `mCricket2ButtonShownPrep` 为 `int`（`-1` 未初始化哨兵），不是 `bool`
