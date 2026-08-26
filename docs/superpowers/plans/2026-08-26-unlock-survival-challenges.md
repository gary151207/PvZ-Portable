# 生存模式页"解锁全部生存关卡"按钮 — 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在挑战模式选择页的生存模式页（`CHALLENGE_PAGE_SURVIVAL`）左下角新增"解锁生存关卡"按钮，点击后内存态解锁该页全部 11 个关卡按钮，不写存档、不算通关。

**Architecture:** 纯 UI 改动，只涉及 `ChallengeScreen` 一个类。新增一个 `NewLawnButton` 成员 + 一个 `bool mCheatUnlockSurvival` 标志；在 `AccomplishmentsNeeded()` 中对该标志做页面限定判断，使生存页挑战返回 0（已解锁）。沿用现有 Back 按钮的生命周期模式（构造创建 / AddedToManager 加入 / RemovedFromManager 移除 / 析构删除）。

**Tech Stack:** C++20, SDL2, SexyAppFramework Widget 系统（ButtonWidget / NewLawnButton / ButtonListener）。

## Global Constraints

- 只修改 `src/Lawn/Widget/ChallengeScreen.h` 和 `src/Lawn/Widget/ChallengeScreen.cpp` 两个文件。
- **不改 `mChallengeRecords`、不调用 `HasBeatenChallenge`、不写玩家存档**（纯内存态解锁）。
- 解锁只对 `mPage == CHALLENGE_PAGE_SURVIVAL` 的挑战生效；小游戏页、益智页、Limbo 页不受影响。
- 按钮**默认总是显示**（不做 Debug 构建门控）。
- 代码风格：`mMember` / `theParameter` 命名，tab 缩进，沿用现有 `mBackButton` 的写法。
- 按钮文案直接硬编码中文字符串（项目已有先例，如 `gChallengeDefs` 中的 `"斗蛐蛐"`）。
- 无自动化测试框架；验证靠 `cmake --build` + 手动 gameplay 测试。

---

### Task 1: 头文件 — 枚举 id 与成员声明

**Files:**
- Modify: `src/Lawn/Widget/ChallengeScreen.h:39-57`

**Interfaces:**
- Consumes: 现有 `NewLawnButton` 前向声明（`class NewLawnButton;` 已在头文件顶部）。
- Produces: 枚举 `ChallengeScreen_UnlockSurvival = 500`；成员 `NewLawnButton* mUnlockSurvivalButton;` 与 `bool mCheatUnlockSurvival;`。后续所有任务使用这两个名字。

- [ ] **Step 1: 在枚举中加入按钮 id**

`ChallengeScreen.h` 私有枚举（约 39-41 行）：

```cpp
    enum
    {
        ChallengeScreen_Back = 100,
        ChallengeScreen_Mode = 200,
        ChallengeScreen_Page = 300,
        ChallengeScreen_UnlockSurvival = 500
    };
```

- [ ] **Step 2: 声明成员**

在 `mCheatEnableChallenges;`（约 51 行）附近加入：

```cpp
    bool                        mCheatEnableChallenges;
    bool                        mCheatUnlockSurvival;
```

在 `mBackButton;`（约 45 行）附近加入：

```cpp
    NewLawnButton*              mBackButton;
    NewLawnButton*              mUnlockSurvivalButton;
```

- [ ] **Step 3: 提交**

```bash
git add src/Lawn/Widget/ChallengeScreen.h
git commit -m "feat(challenge-screen): declare unlock-survival button members"
```

---

### Task 2: 构造函数 — 初始化标志并创建按钮

**Files:**
- Modify: `src/Lawn/Widget/ChallengeScreen.cpp:121-137`

**Interfaces:**
- Consumes: Task 1 的成员声明；现有 `MakeNewButton`（`src/Lawn/Widget/GameButton.h:136`）、`Sexy::IMAGE_SEEDCHOOSER_BUTTON2(_GLOW)`、`ButtonWidget::COLOR_LABEL(_HILITE)`、`Color(42, 42, 90)`。
- Produces: 构造完成后 `mUnlockSurvivalButton` 指向有效按钮，位于 `(140, 568, 111, 26)`，文字"解锁生存关卡"。

- [ ] **Step 1: 初始化 `mCheatUnlockSurvival`**

在构造函数中 `mCheatEnableChallenges = false;`（约 121 行）旁：

```cpp
	mCheatEnableChallenges = false;
	mCheatUnlockSurvival = false;
```

- [ ] **Step 2: 创建按钮**

紧接 `mBackButton->Resize(18, 568, 111, 26);`（约 137 行）之后：

```cpp
	mUnlockSurvivalButton = MakeNewButton(ChallengeScreen::ChallengeScreen_UnlockSurvival, this, "解锁生存关卡", nullptr, Sexy::IMAGE_SEEDCHOOSER_BUTTON2, 
		Sexy::IMAGE_SEEDCHOOSER_BUTTON2_GLOW, Sexy::IMAGE_SEEDCHOOSER_BUTTON2_GLOW);
	mUnlockSurvivalButton->mTextDownOffsetX = 1;
	mUnlockSurvivalButton->mTextDownOffsetY = 1;
	mUnlockSurvivalButton->mColors[ButtonWidget::COLOR_LABEL] = Color(42, 42, 90);
	mUnlockSurvivalButton->mColors[ButtonWidget::COLOR_LABEL_HILITE] = Color(42, 42, 90);
	mUnlockSurvivalButton->Resize(140, 568, 111, 26);
```

- [ ] **Step 3: 提交**

```bash
git add src/Lawn/Widget/ChallengeScreen.cpp
git commit -m "feat(challenge-screen): create unlock-survival button in constructor"
```

---

### Task 3: 解锁逻辑 — `AccomplishmentsNeeded` 页面限定判断

**Files:**
- Modify: `src/Lawn/Widget/ChallengeScreen.cpp:401-408`

**Interfaces:**
- Consumes: Task 1 的 `mCheatUnlockSurvival`；现有 `GetChallengeDefinition()`、`ChallengeDefinition::mPage`、`ChallengePage::CHALLENGE_PAGE_SURVIVAL`。
- Produces: `mCheatUnlockSurvival == true` 时，生存页挑战的 `AccomplishmentsNeeded()` 返回 0。

- [ ] **Step 1: 加入解锁判断**

将 `AccomplishmentsNeeded`（约 401-408 行）改为：

```cpp
int ChallengeScreen::AccomplishmentsNeeded(int theChallengeIndex)
{
	ChallengeDefinition& aDef = GetChallengeDefinition(theChallengeIndex);
	if (mCheatUnlockSurvival && aDef.mPage == ChallengePage::CHALLENGE_PAGE_SURVIVAL)
		return 0;

	int aTrophiesNeeded = MoreTrophiesNeeded(theChallengeIndex);
	GameMode aGameMode = aDef.mChallengeMode;
	if (mApp->IsSurvivalEndless(aGameMode) && aTrophiesNeeded <= 3 && mApp->GetNumTrophies(CHALLENGE_PAGE_SURVIVAL) < 10 &&
		mApp->HasFinishedAdventure() && !mApp->IsTrialStageLocked()) aTrophiesNeeded = 1;
	return mCheatEnableChallenges ? 0 : aTrophiesNeeded;
}
```

> 注意：原函数里 `GetChallengeDefinition(theChallengeIndex)` 被调用两次（一次取 `mChallengeMode`）。改为取局部引用 `aDef`，语义不变。

- [ ] **Step 2: 提交**

```bash
git add src/Lawn/Widget/ChallengeScreen.cpp
git commit -m "feat(challenge-screen): memory-unlock survival page challenges"
```

---

### Task 4: 交互与状态 — `ButtonDepress` 点击处理 + `UpdateButtons` 可见性/禁用

**Files:**
- Modify: `src/Lawn/Widget/ChallengeScreen.cpp:376-399`（UpdateButtons）、`src/Lawn/Widget/ChallengeScreen.cpp:660-682`（ButtonDepress）

**Interfaces:**
- Consumes: Task 1 的成员、Task 2 的按钮、`FOLEY_PAPER`（`src/Sexy.TodLib/TodFoley.h:134`，文件已包含）。
- Produces: 点击按钮后 `mCheatUnlockSurvival = true`、按钮灰显；切页时按钮只在生存页显示。

- [ ] **Step 1: `UpdateButtons` 中控制按钮状态**

在 `UpdateButtons()`（约 376-399 行）开头，第一个 for 循环之前加入：

```cpp
	mUnlockSurvivalButton->mVisible = mPageIndex == CHALLENGE_PAGE_SURVIVAL;
	mUnlockSurvivalButton->mDisabled = mCheatUnlockSurvival;
```

- [ ] **Step 2: `ButtonDepress` 处理点击**

在 `ButtonDepress`（约 660 行）开头，`if (theId == ChallengeScreen::ChallengeScreen_Back)` 分支之前加入：

```cpp
	if (theId == ChallengeScreen::ChallengeScreen_UnlockSurvival)
	{
		if (!mCheatUnlockSurvival)
		{
			mCheatUnlockSurvival = true;
			UpdateButtons();
			mApp->PlayFoley(FOLEY_PAPER);
		}
		return;
	}
```

- [ ] **Step 3: 提交**

```bash
git add src/Lawn/Widget/ChallengeScreen.cpp
git commit -m "feat(challenge-screen): handle unlock-survival button press"
```

---

### Task 5: 生命周期 — 析构、AddedToManager、RemovedFromManager

**Files:**
- Modify: `src/Lawn/Widget/ChallengeScreen.cpp:211-215`（析构）、`src/Lawn/Widget/ChallengeScreen.cpp:638-653`（AddedToManager / RemovedFromManager）

**Interfaces:**
- Consumes: Task 2 创建的 `mUnlockSurvivalButton`。
- Produces: 按钮与 Widget 树正确绑定/解绑，无内存泄漏。

- [ ] **Step 1: 析构删除按钮**

在析构函数 `delete mBackButton;`（约 213 行）旁：

```cpp
	delete mBackButton;
	delete mUnlockSurvivalButton;
```

- [ ] **Step 2: AddedToManager 加入**

在 `AddWidget(mBackButton);`（约 641 行）旁：

```cpp
	AddWidget(mBackButton);
	AddWidget(mUnlockSurvivalButton);
```

- [ ] **Step 3: RemovedFromManager 移除**

在 `RemoveWidget(mBackButton);`（约 649 行）旁：

```cpp
	RemoveWidget(mBackButton);
	RemoveWidget(mUnlockSurvivalButton);
```

- [ ] **Step 4: 提交**

```bash
git add src/Lawn/Widget/ChallengeScreen.cpp
git commit -m "feat(challenge-screen): manage unlock-survival button lifetime"
```

---

### Task 6: 构建与手动验证

**Files:**
- Build: 项目根目录（已有 `build/` 目录）

**Interfaces:**
- Consumes: Task 1-5 的全部改动。
- Produces: 可运行的二进制，通过下述手动测试清单。

- [ ] **Step 1: 构建**

```bash
cmake --build build
```

Expected: 编译成功，无警告/错误（若 `build/` 未配置，先 `cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release`）。

- [ ] **Step 2: 手动测试 — 按钮与解锁**

启动游戏（`run-pvz.bat` 或直接运行 `build/` 下生成的二进制）：
1. 新建存档 → 主菜单 → 挑战模式 → 生存模式页：确认左下角出现"解锁生存关卡"按钮，与 Back 按钮同排（`(140,568)`），锁定的关卡仍显示锁图标。
2. 点击按钮：确认按钮灰显（禁用）、播放解锁音效、生存页 11 个按钮（Normal 5 + Hard 5 + 泳池无尽 1）的锁图标消失、全部可点击。
3. 点击任一刚解锁的生存关卡：确认能正常进入关卡（选卡界面出现）。

- [ ] **Step 3: 手动测试 — 边界**

4. 切换到小游戏页 / 益智页 / Limbo 页：确认"解锁生存关卡"按钮隐藏，且这些页面的锁定状态不受影响（仍按原逻辑锁定/解锁）。
5. 退出到主菜单，再重新进入生存模式页：确认锁定恢复（内存态不持久化）。
6. 查看通关记录：确认 `mChallengeRecords` 未变（无奖杯图标、`HasBeatenChallenge` 仍为 false——即生存关卡图标上不出现奖杯）。

- [ ] **Step 4: 收尾提交（如有测试中发现的问题则修复后提交）**

```bash
git status
```

Expected: 工作区干净（若无额外修改），Task 1-5 的 5 个 commit 均已落地。
