# 无尽模式多存档槽系统 — 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 给生存无尽（5 阶段）、我是僵尸无尽、惊吓盒无尽各加 5 个存档槽位，进入时弹槽位选择对话框，支持命名、改名、删除、继续、开新局。

**Architecture:** 每个槽位两个文件：`.v4` 存档（复用现有 `LawnSaveGame` 序列化）+ `.meta` 纯文本元数据（名称/进度/时间戳，槽位对话框只读它显示，不解析 .v4）。`LawnApp::mEndlessSlotId` 记录当前运行中的槽位，现有自动存档/删除路径据此路由。

**Tech Stack:** C++20、SDL2、CMake（源文件 GLOB 自动收录新文件）、现有 SexyAppFramework Widget/Dialog/ListWidget/EditWidget 控件。

## Global Constraints

- **无自动化测试**（AGENTS.md）：每个任务的验证 = `cmake --build build` 编译通过 + 手动游戏验证。本计划的"测试"步骤均为手动操作清单。
- **代码风格**：`mMember` / `theParameter` / `aLocal` 命名；头文件守卫 `#ifndef __MODULE_H__` / `#define` / `#endif`；每个 `.h`/`.cpp` 必须带 LGPL-3.0-or-later 许可证头（照抄现有文件头块）。
- **ConstEnums.h 不可重排**：新对话框枚举值追加到 `NUM_DIALOGS` 之前，不修改已有值。
- **存档格式不变**：槽位 `.v4` 仍由 `LawnSaveGame`/`LawnLoadGame` 处理，TLV 格式不动。
- **不要给植物/僵尸建子类**：本任务不涉及，无需触碰。
- 编译检查命令：`cmake --build build`（构建目录已存在；若未配置先跑 `cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release`）。
- 设计文档：`docs/superpowers/specs/2026-08-28-endless-multi-slot-save-design.md`（实现须与之对齐；本计划中 CHEAT 与 RESTART 同样处理，是对设计的微小澄清，见 Task 2 说明）。

---

### Task 1: EndlessSlot 存储模块

**Files:**
- Create: `src/Lawn/System/EndlessSlot.h`
- Create: `src/Lawn/System/EndlessSlot.cpp`

**Interfaces:**
- Consumes: `GameMode`（`src/ConstEnums.h`）、`LawnApp::IsSurvivalEndless/IsEndlessIZombie/IsEndlessScaryPotter`（静态成员）、`GetChallengeDefinition`（`src/Lawn/Widget/ChallengeScreen.h`）、`GetSavedGameName`（`src/Lawn/LawnCommon.h`）、`TodStringTranslate`、`StrFormat`、`GetAppDataPath`、`MkDir`、`gSexyAppBase->FileExists/EraseFile`。
- Produces（后续任务依赖的精确签名）:

```cpp
struct EndlessSlotMeta
{
    std::string mName;
    int mFlags = 0;   // 已过旗帜数（生存无尽）
    int mWave = 0;    // 当前波数（生存无尽）
    int mStage = 0;   // 阶段数（谜题无尽）
    std::string mTimestamp;
};

bool        IsEndlessGameMode(GameMode theGameMode);
std::string GetEndlessSaveName(GameMode theGameMode, int theProfileId, int theSlot);
std::string GetEndlessMetaName(GameMode theGameMode, int theProfileId, int theSlot);
std::string GetEndlessAutoName(GameMode theGameMode, int theSlot);
bool        EndlessSlotHasSave(GameMode theGameMode, int theProfileId, int theSlot);
bool        LoadEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, EndlessSlotMeta& theMeta);
void        SaveEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, const EndlessSlotMeta& theMeta);
void        InitEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, const std::string& theName);
void        RefreshEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, int theFlags, int theWave, int theStage);
void        EraseEndlessSlot(GameMode theGameMode, int theProfileId, int theSlot);
void        MigrateLegacyEndlessSave(GameMode theGameMode, int theProfileId);
```

- [ ] **Step 1: 写 `src/Lawn/System/EndlessSlot.h`**

```cpp
/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of PvZ-Portable.
 *
 * PvZ-Portable is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PvZ-Portable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PvZ-Portable. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __ENDLESSSLOT_H__
#define __ENDLESSSLOT_H__

#include <string>

enum GameMode : int32_t;   // 前向声明，见 ConstEnums.h

struct EndlessSlotMeta
{
    std::string mName;
    int mFlags = 0;   // 已过旗帜数（生存无尽）
    int mWave = 0;    // 当前波数（生存无尽）
    int mStage = 0;   // 阶段数（谜题无尽）
    std::string mTimestamp;
};

bool        IsEndlessGameMode(GameMode theGameMode);
std::string GetEndlessSaveName(GameMode theGameMode, int theProfileId, int theSlot);
std::string GetEndlessMetaName(GameMode theGameMode, int theProfileId, int theSlot);
std::string GetEndlessAutoName(GameMode theGameMode, int theSlot);
bool        EndlessSlotHasSave(GameMode theGameMode, int theProfileId, int theSlot);
bool        LoadEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, EndlessSlotMeta& theMeta);
void        SaveEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, const EndlessSlotMeta& theMeta);
void        InitEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, const std::string& theName);
void        RefreshEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, int theFlags, int theWave, int theStage);
void        EraseEndlessSlot(GameMode theGameMode, int theProfileId, int theSlot);
void        MigrateLegacyEndlessSave(GameMode theGameMode, int theProfileId);

#endif
```

> 说明：`GameMode` 是普通 `enum`（带 int32_t 底层类型，ConstEnums.h:377），前向声明写法 `enum GameMode : int32_t;`。若编译器报错，改为在 .h 里 `#include "../../ConstEnums.h"`。

- [ ] **Step 2: 写 `src/Lawn/System/EndlessSlot.cpp`**

```cpp
/* 许可证头同上，照抄 */

#include "EndlessSlot.h"
#include "../../ConstEnums.h"
#include "../../LawnApp.h"
#include "../LawnCommon.h"
#include "../Widget/ChallengeScreen.h"
#include "../../Sexy.TodLib/TodStringFile.h"
#include "../../SexyAppFramework/Common.h"
#include "../../SexyAppFramework/SexyAppBase.h"

#include <cstdio>
#include <ctime>

bool IsEndlessGameMode(GameMode theGameMode)
{
    return LawnApp::IsSurvivalEndless(theGameMode) ||
        LawnApp::IsEndlessIZombie(theGameMode) ||
        LawnApp::IsEndlessScaryPotter(theGameMode);
}

std::string GetEndlessSaveName(GameMode theGameMode, int theProfileId, int theSlot)
{
    return GetAppDataPath(StrFormat("userdata/game%d_%d_s%d.v4", theProfileId, static_cast<int>(theGameMode), theSlot));
}

std::string GetEndlessMetaName(GameMode theGameMode, int theProfileId, int theSlot)
{
    return GetAppDataPath(StrFormat("userdata/game%d_%d_s%d.meta", theProfileId, static_cast<int>(theGameMode), theSlot));
}

std::string GetEndlessAutoName(GameMode theGameMode, int theSlot)
{
    int aChallengeIndex = static_cast<int>(theGameMode) - static_cast<int>(GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_1);
    std::string aModeName = TodStringTranslate(GetChallengeDefinition(aChallengeIndex).mChallengeName);
    return aModeName + StrFormat(" 存档%d", theSlot + 1);
}

bool EndlessSlotHasSave(GameMode theGameMode, int theProfileId, int theSlot)
{
    return gSexyAppBase->FileExists(GetEndlessSaveName(theGameMode, theProfileId, theSlot));
}

static void SetCurrentTimestamp(std::string& theTimestamp)
{
    time_t aNow = time(nullptr);
    char aBuf[64];
    strftime(aBuf, sizeof(aBuf), "%Y-%m-%d %H:%M", localtime(&aNow));
    theTimestamp = aBuf;
}

bool LoadEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, EndlessSlotMeta& theMeta)
{
    std::string aFilePath = GetEndlessMetaName(theGameMode, theProfileId, theSlot);
    FILE* aFile = fopen(aFilePath.c_str(), "rb");
    if (!aFile)
        return false;

    theMeta = EndlessSlotMeta();
    char aNameBuf[512];
    if (fgets(aNameBuf, sizeof(aNameBuf), aFile))
    {
        std::string aName = aNameBuf;
        if (!aName.empty() && (aName[aName.size() - 1] == '\n' || aName[aName.size() - 1] == '\r'))
            aName.resize(aName.size() - 1);
        if (!aName.empty() && aName[aName.size() - 1] == '\r')
            aName.resize(aName.size() - 1);
        theMeta.mName = aName;
    }

    char aLine[256];
    while (fgets(aLine, sizeof(aLine), aFile))
    {
        std::string aLineStr = aLine;
        if (!aLineStr.empty() && (aLineStr[aLineStr.size() - 1] == '\n' || aLineStr[aLineStr.size() - 1] == '\r'))
            aLineStr.resize(aLineStr.size() - 1);
        int aValue = 0;
        if (sscanf(aLineStr.c_str(), "flags=%d", &aValue) == 1)
            theMeta.mFlags = aValue;
        else if (sscanf(aLineStr.c_str(), "wave=%d", &aValue) == 1)
            theMeta.mWave = aValue;
        else if (sscanf(aLineStr.c_str(), "stage=%d", &aValue) == 1)
            theMeta.mStage = aValue;
        else if (aLineStr.compare(0, 10, "timestamp=") == 0)
            theMeta.mTimestamp = aLineStr.substr(10);
    }

    fclose(aFile);
    return true;
}

void SaveEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, const EndlessSlotMeta& theMeta)
{
    MkDir(GetAppDataPath("userdata"));
    std::string aFilePath = GetEndlessMetaName(theGameMode, theProfileId, theSlot);
    FILE* aFile = fopen(aFilePath.c_str(), "wb");
    if (!aFile)
        return;
    fprintf(aFile, "%s\n", theMeta.mName.c_str());
    fprintf(aFile, "flags=%d\n", theMeta.mFlags);
    fprintf(aFile, "wave=%d\n", theMeta.mWave);
    fprintf(aFile, "stage=%d\n", theMeta.mStage);
    fprintf(aFile, "timestamp=%s\n", theMeta.mTimestamp.c_str());
    fclose(aFile);
}

void InitEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, const std::string& theName)
{
    EndlessSlotMeta aMeta;
    aMeta.mName = theName;
    SetCurrentTimestamp(aMeta.mTimestamp);
    SaveEndlessSlotMeta(theGameMode, theProfileId, theSlot, aMeta);
}

void RefreshEndlessSlotMeta(GameMode theGameMode, int theProfileId, int theSlot, int theFlags, int theWave, int theStage)
{
    EndlessSlotMeta aMeta;
    if (!LoadEndlessSlotMeta(theGameMode, theProfileId, theSlot, aMeta))
        aMeta.mName = GetEndlessAutoName(theGameMode, theSlot);
    aMeta.mFlags = theFlags;
    aMeta.mWave = theWave;
    aMeta.mStage = theStage;
    SetCurrentTimestamp(aMeta.mTimestamp);
    SaveEndlessSlotMeta(theGameMode, theProfileId, theSlot, aMeta);
}

void EraseEndlessSlot(GameMode theGameMode, int theProfileId, int theSlot)
{
    gSexyAppBase->EraseFile(GetEndlessSaveName(theGameMode, theProfileId, theSlot));
    gSexyAppBase->EraseFile(GetEndlessMetaName(theGameMode, theProfileId, theSlot));
}

void MigrateLegacyEndlessSave(GameMode theGameMode, int theProfileId)
{
    if (!IsEndlessGameMode(theGameMode))
        return;
    std::string aLegacyName = GetSavedGameName(theGameMode, theProfileId);
    std::string aSlot0Name = GetEndlessSaveName(theGameMode, theProfileId, 0);
    if (!gSexyAppBase->FileExists(aLegacyName) || gSexyAppBase->FileExists(aSlot0Name))
        return;

    // 读旧文件字节，写入槽位 0，再删旧文件（跨平台无 rename，用读写搬移）
    Buffer aBuffer;
    if (gSexyAppBase->ReadBufferFromFile(aLegacyName, &aBuffer, false) && aBuffer.GetDataLen() > 0)
    {
        FILE* aOut = fopen(aSlot0Name.c_str(), "wb");
        if (aOut)
        {
            fwrite(aBuffer.GetDataPtr(), 1, static_cast<size_t>(aBuffer.GetDataLen()), aOut);
            fclose(aOut);
            gSexyAppBase->EraseFile(aLegacyName);
        }
    }
}
```

> 说明：`Buffer` 来自 `SexyAppBase.h`（已 include），`ReadBufferFromFile`/`GetDataLen`/`GetDataPtr` 均为此类成员；`Buffer` 类型在 `Common.h` 中定义。若 `Buffer` 未随 SexyAppBase.h 引入，追加 `#include "../../SexyAppFramework/Buffer.h"`（以 `grep -rn "class Buffer" src/SexyAppFramework/` 实际位置为准）。

- [ ] **Step 3: 编译检查**

Run: `cmake --build build`
Expected: 编译通过（GLOB 自动收录新文件）。若报 `GameMode` 未定义，把 EndlessSlot.h 的前向声明换成 `#include "../../ConstEnums.h"`；若报 `Buffer` 未定义，按 Step 2 末尾说明补 include。

- [ ] **Step 4: 提交**

```bash
git add src/Lawn/System/EndlessSlot.h src/Lawn/System/EndlessSlot.cpp
git commit -m "feat(endless-save): add EndlessSlot storage module (meta read/write, migrate, erase)"
```

---

### Task 2: LawnApp 槽位路由与存档生命周期

**Files:**
- Modify: `src/LawnApp.h`（新增成员 `mEndlessSlotId`）
- Modify: `src/LawnApp.cpp`（构造函数初始化；`KillBoard` 删除分支；`PreNewGame` 清除分支）
- Modify: `src/Lawn/Board.cpp`（`TryToSaveGame` 路由到槽位文件 + 存档成功后刷新 `.meta`）

**Interfaces:**
- Consumes: Task 1 的 `IsEndlessGameMode` / `GetEndlessSaveName` / `EraseEndlessSlot` / `RefreshEndlessSlotMeta`。
- Produces: 成员 `int mEndlessSlotId`（-1 = 非无尽/未选）。Task 4 的 `LoadEndlessSlot` / `StartEndlessNewGame` 会写它。

> **CHEAT 与 RESTART 的澄清**（相对设计文档）：`FinishCheatDialog` 不经过 `KillBoard`，而是直接 `PreNewGame(mode,false)`。为避免槽位状态不一致，CHEAT 与 RESTART 同样处理——**只删 `.v4`、保留 `.meta` 名字、同槽重开**。只有 LOST/WON 清空整个槽位。

- [ ] **Step 1: `src/LawnApp.h` 新增成员**

在 `int mBoardResult;`（约 122 行）下方加：

```cpp
	int								mBoardResult;
	int								mEndlessSlotId;   // 无尽模式当前运行的槽位，-1 = 非无尽/未选
```

- [ ] **Step 2: `src/LawnApp.cpp` 构造函数初始化**

在构造函数中 `mBoardResult` 初始化处附近（搜 `mBoardResult =`，构造体内）追加：

```cpp
	mEndlessSlotId = -1;
```

若构造函数里没有 `mBoardResult` 初始化行，就加在 `mCricketMatchCount = 0;` 之类成员初始化附近任意位置。

- [ ] **Step 3: `src/LawnApp.cpp` 顶部加 include**

```cpp
#include "Lawn/System/EndlessSlot.h"
```

（与现有 `#include "Lawn/Widget/ContinueDialog.h"` 并列）

- [ ] **Step 4: `KillBoard` 删除分支按槽位路由**

替换 `LawnApp::KillBoard()`（约 335-349 行）内的这段：

```cpp
		if (mPlayerInfo && (
			mBoardResult == BoardResult::BOARDRESULT_WON ||
			mBoardResult == BoardResult::BOARDRESULT_LOST ||
			mBoardResult == BoardResult::BOARDRESULT_RESTART ||
			mBoardResult == BoardResult::BOARDRESULT_CHEAT))
		{
			std::string aFileName = GetSavedGameName(mGameMode, mPlayerInfo->mId);
			EraseFile(aFileName);
			std::string aLegacyFileName = GetLegacySavedGameName(mGameMode, mPlayerInfo->mId);
			EraseFile(aLegacyFileName);
		}
```

改为：

```cpp
		if (mPlayerInfo && (
			mBoardResult == BoardResult::BOARDRESULT_WON ||
			mBoardResult == BoardResult::BOARDRESULT_LOST ||
			mBoardResult == BoardResult::BOARDRESULT_RESTART ||
			mBoardResult == BoardResult::BOARDRESULT_CHEAT))
		{
			std::string aLegacyFileName = GetLegacySavedGameName(mGameMode, mPlayerInfo->mId);
			EraseFile(aLegacyFileName);
			if (IsEndlessGameMode(mGameMode) && mEndlessSlotId >= 0)
			{
				if (mBoardResult == BoardResult::BOARDRESULT_RESTART || mBoardResult == BoardResult::BOARDRESULT_CHEAT)
				{
					// 同槽重开：只删 .v4，保留 .meta 名字
					EraseFile(GetEndlessSaveName(mGameMode, mPlayerInfo->mId, mEndlessSlotId));
				}
				else
				{
					// 失败/通关：整槽清空
					EraseEndlessSlot(mGameMode, mPlayerInfo->mId, mEndlessSlotId);
					mEndlessSlotId = -1;
				}
			}
			else
			{
				std::string aFileName = GetSavedGameName(mGameMode, mPlayerInfo->mId);
				EraseFile(aFileName);
			}
		}
```

- [ ] **Step 5: `PreNewGame` 清除分支按槽位路由**

替换 `LawnApp::PreNewGame()`（约 429-440 行）内的：

```cpp
	std::string aFileName = GetSavedGameName(mGameMode, mPlayerInfo->mId);
	EraseFile(aFileName);
	std::string aLegacyFileName = GetLegacySavedGameName(mGameMode, mPlayerInfo->mId);
	EraseFile(aLegacyFileName);
```

改为：

```cpp
	std::string aFileName = GetSavedGameName(mGameMode, mPlayerInfo->mId);
	if (IsEndlessGameMode(mGameMode) && mEndlessSlotId >= 0)
		aFileName = GetEndlessSaveName(mGameMode, mPlayerInfo->mId, mEndlessSlotId);
	EraseFile(aFileName);
	std::string aLegacyFileName = GetLegacySavedGameName(mGameMode, mPlayerInfo->mId);
	EraseFile(aLegacyFileName);
```

- [ ] **Step 6: `Board::TryToSaveGame` 路由 + 写元数据**

`src/Lawn/Board.cpp` 顶部加 include（与 `#include "System/SaveGame.h"` 并列）：

```cpp
#include "System/EndlessSlot.h"
```

替换 `Board::TryToSaveGame()`（约 377-396 行）为：

```cpp
void Board::TryToSaveGame()
{
	bool aIsEndless = IsEndlessGameMode(mApp->mGameMode) && mApp->mEndlessSlotId >= 0;
	std::string aFileName = GetSavedGameName(mApp->mGameMode, mApp->mPlayerInfo->mId);
	if (aIsEndless)
		aFileName = GetEndlessSaveName(mApp->mGameMode, mApp->mPlayerInfo->mId, mApp->mEndlessSlotId);

	if (NeedSaveGame())
	{
		if (mBoardFadeOutCounter > 0)
		{
			CompleteEndLevelSequenceForSaving();
			return;
		}

		MkDir(GetAppDataPath("userdata"));
		mApp->mMusic->GameMusicPause(true);
		if (LawnSaveGame(this, aFileName))
		{
			if (aIsEndless)
			{
				int aFlags = mApp->IsSurvivalEndless(mApp->mGameMode) ? GetSurvivalFlagsCompleted() : 0;
				int aStage = mChallenge ? mChallenge->mSurvivalStage : 0;
				RefreshEndlessSlotMeta(mApp->mGameMode, mApp->mPlayerInfo->mId, mApp->mEndlessSlotId, aFlags, mCurrentWave, aStage);
			}
		}
		mApp->ClearUpdateBacklog();
		SurvivalSaveScore();
	}
}
```

- [ ] **Step 7: 编译检查**

Run: `cmake --build build`
Expected: 编译通过。

- [ ] **Step 8: 手动冒烟**

Run: `./run-pvz.sh`（或 Windows 下 `run-pvz.bat`），进入"挑战模式 → 生存模式页"，点泳池无尽（SURVIVAL_ENDLESS_STAGE_3）。预期：行为不变（此时还是原单槽流程，槽位对话框在 Task 4 才接入），但暂停存档时若 `mEndlessSlotId == -1` 走老逻辑。确认能正常进入关卡、暂停不崩溃即可。
Expected: 无崩溃、无异常。

- [ ] **Step 9: 提交**

```bash
git add src/LawnApp.h src/LawnApp.cpp src/Lawn/Board.cpp
git commit -m "feat(endless-save): route autosave/erase to active endless slot via mEndlessSlotId"
```

---

### Task 3: 对话框枚举 + 命名对话框

**Files:**
- Modify: `src/ConstEnums.h`（追加 3 个对话框 id）
- Create: `src/Lawn/Widget/EndlessNameDialog.h`
- Create: `src/Lawn/Widget/EndlessNameDialog.cpp`
- Modify: `src/LawnApp.h`（声明命名对话框函数）
- Modify: `src/LawnApp.cpp`（实现 + ButtonDepress 分发）

**Interfaces:**
- Consumes: `Dialogs` 枚举、`CreateEditWidget` / `LawnEditWidget` / `DrawEditBox`（LawnCommon.h）、`FONT_BRIANNETOD16`（Resources.h）、`EditListener`。
- Produces:
  - `void LawnApp::DoEndlessNameDialog(const std::string& thePrefill, bool isRename);`
  - `void LawnApp::FinishEndlessNameDialog(bool isYes);`
  - `class EndlessNameDialog`（ctor `(LawnApp*, const std::string& thePrefill, bool isRename)`、`std::string GetName()`）

- [ ] **Step 1: `src/ConstEnums.h` 追加对话框 id**

在 `Dialogs` 枚举的 `NUM_DIALOGS` 之前（`DIALOG_PURCHASE_PACKET_SLOT` 之后）加：

```cpp
    DIALOG_ENDLESSSLOT,                         // 51：无尽模式存档槽选择
    DIALOG_ENDLESSNAME,                         // 52：无尽模式存档命名
    DIALOG_ENDLESSDELETE,                       // 53：无尽模式存档删除确认
    NUM_DIALOGS
```

（把原有 `NUM_DIALOGS` 行替换为上面四行）

- [ ] **Step 2: 写 `src/Lawn/Widget/EndlessNameDialog.h`**

```cpp
/* 许可证头照抄 Task 1 的，换文件名注释可不写 */

#ifndef __ENDLESSNAMEDIALOG_H__
#define __ENDLESSNAMEDIALOG_H__

#include "LawnDialog.h"
#include "widget/EditListener.h"

class LawnEditWidget;

class EndlessNameDialog : public LawnDialog, public EditListener
{
public:
	LawnEditWidget*		mNameEditWidget;
	bool				mIsRename;

public:
	EndlessNameDialog(LawnApp* theApp, const std::string& thePrefill, bool isRename);
	virtual ~EndlessNameDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager);
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager);
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight);
	virtual int			GetPreferredHeight(int theWidth);
	virtual void		Draw(Graphics* g);
	virtual void		EditWidgetText(int theId, const std::string& theString);
	virtual bool		AllowChar(int theId, char theChar);
	std::string			GetName();
	void				SetName(const std::string& theName);
};

#endif
```

- [ ] **Step 3: 写 `src/Lawn/Widget/EndlessNameDialog.cpp`**

```cpp
/* 许可证头照抄 */

#include "EndlessNameDialog.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "widget/WidgetManager.h"

EndlessNameDialog::EndlessNameDialog(LawnApp* theApp, const std::string& thePrefill, bool isRename) : LawnDialog(
	theApp,
	Dialogs::DIALOG_ENDLESSNAME,
	true,
	isRename ? "重命名存档" : "新存档",
	"请输入存档名称：",
	"[DIALOG_BUTTON_OK]",
	Dialog::BUTTONS_OK_CANCEL)
{
	mApp = theApp;
	mIsRename = isRename;
	mVerticalCenterText = false;
	mNameEditWidget = CreateEditWidget(0, this, this);
	mNameEditWidget->mMaxChars = 24;
	mNameEditWidget->AddWidthCheckFont(FONT_BRIANNETOD16, 220);
	CalcSize(110, 40);
	SetName(thePrefill);
}

EndlessNameDialog::~EndlessNameDialog()
{
	delete mNameEditWidget;
}

void EndlessNameDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	LawnDialog::AddedToManager(theWidgetManager);
	AddWidget(mNameEditWidget);
	theWidgetManager->SetFocus(mNameEditWidget);
}

void EndlessNameDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	LawnDialog::RemovedFromManager(theWidgetManager);
	RemoveWidget(mNameEditWidget);
}

int EndlessNameDialog::GetPreferredHeight(int theWidth)
{
	return LawnDialog::GetPreferredHeight(theWidth) + 40;
}

void EndlessNameDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	LawnDialog::Resize(theX, theY, theWidth, theHeight);
	mNameEditWidget->Resize(mContentInsets.mLeft + 12, mHeight - 155, mWidth - mContentInsets.mLeft - mContentInsets.mRight - 24, 28);
}

void EndlessNameDialog::Draw(Graphics* g)
{
	LawnDialog::Draw(g);
	DrawEditBox(g, mNameEditWidget);
}

void EndlessNameDialog::EditWidgetText(int theId, const std::string& theString)
{
	(void)theId;
	(void)theString;
	mApp->ButtonDepress(mId + 2000);
}

bool EndlessNameDialog::AllowChar(int, char theChar)
{
	// 允许任意可打印字符（含 UTF-8 中文），拒绝控制字符
	return theChar >= 0x20;
}

std::string EndlessNameDialog::GetName()
{
	std::string aString;
	char aLastChar = ' ';

	for (size_t i = 0; i < mNameEditWidget->mString.size(); i++)
	{
		char aChar = mNameEditWidget->mString[i];
		if (aChar != ' ')
		{
			aString.append(1, aChar);
		}
		else if (aChar != aLastChar)
		{
			aString.append(1, ' ');
		}

		aLastChar = aChar;
	}

	if (aString.size() && aString[aString.size() - 1] == ' ')
	{
		aString.resize(aString.size() - 1);
	}

	return aString;
}

void EndlessNameDialog::SetName(const std::string& theName)
{
	mNameEditWidget->SetText(theName, true);
	mNameEditWidget->mCursorPos = static_cast<int>(theName.size());
	mNameEditWidget->mHilitePos = 0;
}
```

> 说明：`mNameEditWidget->mString` 是 `std::string`（EditWidget 成员），`mCursorPos`/`mHilitePos` 为 EditWidget 成员；照抄 NewUserDialog.cpp 的用法。若 `mCursorPos` 类型不是 int，去掉 `static_cast<int>` 直接赋值。

- [ ] **Step 4: `src/LawnApp.h` 声明命名对话框函数**

在 `DoContinueDialog();` 附近加：

```cpp
	void							DoEndlessNameDialog(const std::string& thePrefill, bool isRename);
	void							FinishEndlessNameDialog(bool isYes);
```

- [ ] **Step 5: `src/LawnApp.cpp` 实现**

顶部 include（与 ContinueDialog 的 include 并列）：

```cpp
#include "Lawn/Widget/EndlessNameDialog.h"
```

在 `DoContinueDialog()` 附近加实现：

```cpp
void LawnApp::DoEndlessNameDialog(const std::string& thePrefill, bool isRename)
{
	KillDialog(Dialogs::DIALOG_ENDLESSNAME);

	EndlessNameDialog* aDialog = new EndlessNameDialog(this, thePrefill, isRename);
	CenterDialog(aDialog, aDialog->mWidth, aDialog->mHeight);
	AddDialog(Dialogs::DIALOG_ENDLESSNAME, aDialog);
}

void LawnApp::FinishEndlessNameDialog(bool isYes)
{
	EndlessNameDialog* aNameDialog = (EndlessNameDialog*)GetDialog(Dialogs::DIALOG_ENDLESSNAME);
	if (aNameDialog == nullptr)
		return;

	if (!isYes)
	{
		KillDialog(Dialogs::DIALOG_ENDLESSNAME);
		return;
	}

	std::string aName = aNameDialog->GetName();
	if (aName.empty())
		return;   // 名字为空：留在命名框里

	KillDialog(Dialogs::DIALOG_ENDLESSNAME);

	// 把名字交回给槽位对话框（Task 4 实现 CommitName）
	EndlessSlotDialog* aSlotDialog = (EndlessSlotDialog*)GetDialog(Dialogs::DIALOG_ENDLESSSLOT);
	if (aSlotDialog)
		aSlotDialog->CommitName(aName);
}
```

> 注意：此处引用了 `EndlessSlotDialog`，Task 4 才创建该类。为让 Task 3 单独可编译，在 Task 3 里先加一个最小前向声明：`src/LawnApp.cpp` 顶部加 `class EndlessSlotDialog;`。Task 4 创建该类后删除这行，改用真实 include。

- [ ] **Step 6: `ButtonDepress` 分发加 case**

在 `LawnApp::ButtonDepress` 的 2000 分支 switch 里（`case Dialogs::DIALOG_TIMESUP:` 附近）加：

```cpp
		case Dialogs::DIALOG_ENDLESSNAME:
			FinishEndlessNameDialog(true);
			return;
```

在 3000 分支 switch 里加：

```cpp
		case Dialogs::DIALOG_ENDLESSNAME:
			FinishEndlessNameDialog(false);
			return;
```

- [ ] **Step 7: 编译检查**

Run: `cmake --build build`
Expected: 编译通过。

- [ ] **Step 8: 提交**

```bash
git add src/ConstEnums.h src/Lawn/Widget/EndlessNameDialog.h src/Lawn/Widget/EndlessNameDialog.cpp src/LawnApp.h src/LawnApp.cpp
git commit -m "feat(endless-save): add EndlessNameDialog and LawnApp name-dialog plumbing"
```

---

### Task 4: 槽位选择对话框 + 无尽入口接入

**Files:**
- Create: `src/Lawn/Widget/EndlessSlotDialog.h`
- Create: `src/Lawn/Widget/EndlessSlotDialog.cpp`
- Modify: `src/LawnApp.h`（声明槽位对话框函数）
- Modify: `src/LawnApp.cpp`（实现 + ButtonDepress 分发 + 删掉 Task 3 的前向声明）
- Modify: `src/Lawn/Widget/ChallengeScreen.cpp`（无尽入口改为弹槽位对话框）

**Interfaces:**
- Consumes: Task 1 全部函数、Task 3 的 `DoEndlessNameDialog` / `FinishEndlessNameDialog`、`mEndlessSlotId`。
- Produces:
  - `void LawnApp::DoEndlessSlotDialog(GameMode theGameMode);`
  - `bool LawnApp::LoadEndlessSlot(GameMode theGameMode, int theSlot);`
  - `void LawnApp::StartEndlessNewGame(GameMode theGameMode, int theSlot, const std::string& theName);`
  - `void LawnApp::DoConfirmDeleteEndlessSlot(int theSlot);`
  - `void LawnApp::FinishConfirmDeleteEndlessSlot(bool isYes);`
  - `class EndlessSlotDialog`（ctor `(LawnApp*, GameMode)`、`void CommitName(const std::string&)`、`void DeleteSelectedSlot()`）

- [ ] **Step 1: 写 `src/Lawn/Widget/EndlessSlotDialog.h`**

```cpp
/* 许可证头照抄 */

#ifndef __ENDLESSSLOTDIALOG_H__
#define __ENDLESSSLOTDIALOG_H__

#include "LawnDialog.h"
#include "widget/ListListener.h"

class LawnApp;
class ListWidget;
class DialogButton;

class EndlessSlotDialog : public LawnDialog, public ListListener
{
protected:
	enum
	{
		EndlessSlotDialog_Start,
		EndlessSlotDialog_Rename,
		EndlessSlotDialog_Delete
	};

public:
	LawnApp*			mApp;
	GameMode			mGameMode;
	ListWidget*			mSlotList;
	DialogButton*		mStartButton;
	DialogButton*		mRenameButton;
	DialogButton*		mDeleteButton;
	int					mNamePendingSlot;      // 正在命名的槽位
	bool				mNamePendingStartNew;  // true = 命名后开新局；false = 命名后仅改名
	int					mPendingDeleteSlot;    // 等待确认删除的槽位

public:
	EndlessSlotDialog(LawnApp* theApp, GameMode theGameMode);
	virtual ~EndlessSlotDialog();

	virtual void		Resize(int theX, int theY, int theWidth, int theHeight);
	virtual int			GetPreferredHeight(int theWidth);
	virtual void		AddedToManager(WidgetManager* theWidgetManager);
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager);
	virtual void		ListClicked(int theId, int theIdx, int theClickCount);
	virtual void		ListClosed(int) {}
	virtual void		ListHiliteChanged(int, int, int) {}
	virtual void		ButtonDepress(int theId);
	void				CommitName(const std::string& theName);
	void				DeleteSelectedSlot();

private:
	int					GetSelectedSlot();
	bool				IsSlotOccupied(int theSlot);
	std::string			GetSlotLabel(int theSlot);
	void				RefreshList();
	void				UpdateButtons();
};

#endif
```

- [ ] **Step 2: 写 `src/Lawn/Widget/EndlessSlotDialog.cpp`**

```cpp
/* 许可证头照抄 */

#include "EndlessSlotDialog.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "../../ConstEnums.h"
#include "../System/EndlessSlot.h"
#include "../../SexyAppFramework/widget/ListWidget.h"
#include "../../SexyAppFramework/Common.h"

static int gEndlessSlotListWidgetColors[][3] = {
	{  23,  24,  35 },
	{   0,   0,   0 },
	{ 235, 225, 180 },
	{ 255, 255, 255 },
	{  20, 180,  15 }
};

EndlessSlotDialog::EndlessSlotDialog(LawnApp* theApp, GameMode theGameMode) : LawnDialog(
	theApp,
	Dialogs::DIALOG_ENDLESSSLOT,
	true,
	"选择存档",
	"选择一个无尽模式存档：",
	"[DIALOG_BUTTON_CANCEL]",
	Dialog::BUTTONS_FOOTER)
{
	mApp = theApp;
	mGameMode = theGameMode;
	mNamePendingSlot = -1;
	mNamePendingStartNew = false;
	mPendingDeleteSlot = -1;

	mVerticalCenterText = false;
	mSlotList = new ListWidget(0, FONT_BRIANNETOD16, this);
	mSlotList->SetColors(gEndlessSlotListWidgetColors, LENGTH(gEndlessSlotListWidgetColors));
	mSlotList->mDrawOutline = true;
	mSlotList->mJustify = ListWidget::JUSTIFY_LEFT;
	mSlotList->mItemHeight = 30;

	mStartButton = MakeButton(EndlessSlotDialog_Start, this, "开始/继续");
	mRenameButton = MakeButton(EndlessSlotDialog_Rename, this, "改名");
	mDeleteButton = MakeButton(EndlessSlotDialog_Delete, this, "删除");

	mTallBottom = true;
	CalcSize(210, 190);
	RefreshList();
	UpdateButtons();
}

EndlessSlotDialog::~EndlessSlotDialog()
{
	delete mSlotList;
	delete mStartButton;
	delete mRenameButton;
	delete mDeleteButton;
}

void EndlessSlotDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	LawnDialog::Resize(theX, theY, theWidth, theHeight);
	mSlotList->Resize(GetLeft() + 30, GetTop() + 4, GetWidth() - 60, 190);
	mStartButton->Layout(LayoutFlags::LAY_SameLeft | LayoutFlags::LAY_Above | LayoutFlags::LAY_SameHeight | LayoutFlags::LAY_SameWidth, mLawnYesButton, 0, 0, 0, 0);
	mRenameButton->Layout(LayoutFlags::LAY_SameLeft | LayoutFlags::LAY_Above | LayoutFlags::LAY_SameHeight | LayoutFlags::LAY_SameWidth, mLawnNoButton, 0, 0, 0, 0);
	mDeleteButton->Layout(LayoutFlags::LAY_SameLeft | LayoutFlags::LAY_Above | LayoutFlags::LAY_SameHeight | LayoutFlags::LAY_SameWidth, mLawnNoButton, 0, -30, 0, 0);
}

int EndlessSlotDialog::GetPreferredHeight(int theWidth)
{
	return LawnDialog::GetPreferredHeight(theWidth) + 190;
}

void EndlessSlotDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	LawnDialog::AddedToManager(theWidgetManager);
	AddWidget(mSlotList);
	AddWidget(mStartButton);
	AddWidget(mRenameButton);
	AddWidget(mDeleteButton);
}

void EndlessSlotDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	LawnDialog::RemovedFromManager(theWidgetManager);
	RemoveWidget(mSlotList);
	RemoveWidget(mStartButton);
	RemoveWidget(mRenameButton);
	RemoveWidget(mDeleteButton);
}

int EndlessSlotDialog::GetSelectedSlot()
{
	return mSlotList->mSelectIdx;
}

bool EndlessSlotDialog::IsSlotOccupied(int theSlot)
{
	return theSlot >= 0 && theSlot < 5 && EndlessSlotHasSave(mGameMode, mApp->mPlayerInfo->mId, theSlot);
}

std::string EndlessSlotDialog::GetSlotLabel(int theSlot)
{
	EndlessSlotMeta aMeta;
	if (LoadEndlessSlotMeta(mGameMode, mApp->mPlayerInfo->mId, theSlot, aMeta))
	{
		std::string aLabel = aMeta.mName;
		if (mApp->IsSurvivalEndless(mGameMode))
			aLabel += StrFormat(" — 已过 %d 旗 · 第 %d 波", aMeta.mFlags, aMeta.mWave);
		else
			aLabel += StrFormat(" — 第 %d 阶段", aMeta.mStage);
		return aLabel;
	}
	if (EndlessSlotHasSave(mGameMode, mApp->mPlayerInfo->mId, theSlot))
		return GetEndlessAutoName(mGameMode, theSlot);   // .v4 存在但 .meta 缺失：显示默认名
	return StrFormat("空槽位 %d", theSlot + 1);
}

void EndlessSlotDialog::RefreshList()
{
	mSlotList->RemoveAll();
	for (int i = 0; i < 5; i++)
	{
		mSlotList->AddLine(GetSlotLabel(i), false);
	}
	if (mSlotList->GetLineCount() > 0)
		mSlotList->SetSelect(0);
	UpdateButtons();
}

void EndlessSlotDialog::UpdateButtons()
{
	bool aOccupied = IsSlotOccupied(GetSelectedSlot());
	mRenameButton->mDisabled = !aOccupied;
	mDeleteButton->mDisabled = !aOccupied;
}

void EndlessSlotDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
	(void)theId;
	mSlotList->SetSelect(theIdx);
	UpdateButtons();
	if (theClickCount == 2)
	{
		ButtonDepress(EndlessSlotDialog_Start);
	}
}

void EndlessSlotDialog::ButtonDepress(int theId)
{
	LawnDialog::ButtonDepress(theId);
	int aSlot = GetSelectedSlot();

	switch (theId)
	{
	case EndlessSlotDialog_Start:
		if (aSlot < 0)
			break;
		if (IsSlotOccupied(aSlot))
		{
			if (!mApp->LoadEndlessSlot(mGameMode, aSlot))
			{
				mApp->DoDialog(
					Dialogs::DIALOG_MESSAGE,
					true,
					"读取失败",
					"该存档已损坏，无法读取。你可以删除这个存档后重新开始。",
					"[DIALOG_BUTTON_OK]",
					Dialog::BUTTONS_FOOTER);
			}
		}
		else
		{
			mNamePendingSlot = aSlot;
			mNamePendingStartNew = true;
			mApp->DoEndlessNameDialog(GetEndlessAutoName(mGameMode, aSlot), false);
		}
		break;

	case EndlessSlotDialog_Rename:
		if (aSlot >= 0 && IsSlotOccupied(aSlot))
		{
			mNamePendingSlot = aSlot;
			mNamePendingStartNew = false;
			std::string aPrefill = GetEndlessAutoName(mGameMode, aSlot);
			EndlessSlotMeta aMeta;
			if (LoadEndlessSlotMeta(mGameMode, mApp->mPlayerInfo->mId, aSlot, aMeta))
				aPrefill = aMeta.mName;
			mApp->DoEndlessNameDialog(aPrefill, true);
		}
		break;

	case EndlessSlotDialog_Delete:
		if (aSlot >= 0 && IsSlotOccupied(aSlot))
		{
			mPendingDeleteSlot = aSlot;
			mApp->DoConfirmDeleteEndlessSlot(aSlot);
		}
		break;
	}
}

void EndlessSlotDialog::CommitName(const std::string& theName)
{
	int aSlot = mNamePendingSlot;
	if (aSlot < 0 || aSlot >= 5)
		return;

	if (mNamePendingStartNew)
	{
		mApp->StartEndlessNewGame(mGameMode, aSlot, theName);
	}
	else
	{
		EndlessSlotMeta aMeta;
		aMeta.mName = theName;
		if (!LoadEndlessSlotMeta(mGameMode, mApp->mPlayerInfo->mId, aSlot, aMeta))
		{
			aMeta.mName = theName;
			aMeta.mFlags = 0;
			aMeta.mWave = 0;
			aMeta.mStage = 0;
		}
		aMeta.mName = theName;
		SaveEndlessSlotMeta(mGameMode, mApp->mPlayerInfo->mId, aSlot, aMeta);
		RefreshList();
	}
	mNamePendingSlot = -1;
	mNamePendingStartNew = false;
}

void EndlessSlotDialog::DeleteSelectedSlot()
{
	int aSlot = mPendingDeleteSlot;
	if (aSlot < 0 || aSlot >= 5)
		return;
	EraseEndlessSlot(mGameMode, mApp->mPlayerInfo->mId, aSlot);
	mPendingDeleteSlot = -1;
	RefreshList();
}
```

- [ ] **Step 3: `src/LawnApp.h` 声明槽位对话框函数**

在 `DoContinueDialog();` 附近加：

```cpp
	void							DoEndlessSlotDialog(GameMode theGameMode);
	bool							LoadEndlessSlot(GameMode theGameMode, int theSlot);
	void							StartEndlessNewGame(GameMode theGameMode, int theSlot, const std::string& theName);
	void							DoConfirmDeleteEndlessSlot(int theSlot);
	void							FinishConfirmDeleteEndlessSlot(bool isYes);
```

- [ ] **Step 4: `src/LawnApp.cpp` 实现**

- 把 Task 3 加的 `class EndlessSlotDialog;` 前向声明**替换**为 include：

```cpp
#include "Lawn/Widget/EndlessSlotDialog.h"
```

- 在 `DoContinueDialog()` 附近加实现（`<ctime>` 已由现有 include 链提供；若报错则顶部加 `#include <ctime>`）：

```cpp
void LawnApp::DoEndlessSlotDialog(GameMode theGameMode)
{
	KillDialog(Dialogs::DIALOG_ENDLESSSLOT);
	MigrateLegacyEndlessSave(theGameMode, mPlayerInfo->mId);

	EndlessSlotDialog* aDialog = new EndlessSlotDialog(this, theGameMode);
	CenterDialog(aDialog, aDialog->mWidth, aDialog->mHeight);
	AddDialog(Dialogs::DIALOG_ENDLESSSLOT, aDialog);
}

bool LawnApp::LoadEndlessSlot(GameMode theGameMode, int theSlot)
{
	mEndlessSlotId = theSlot;
	std::string aSaveName = GetEndlessSaveName(theGameMode, mPlayerInfo->mId, theSlot);
	mMusic->StopAllMusic();
	if (!FileExists(aSaveName))
		return false;

	KillChallengeScreen();
	MakeNewBoard();
	if (mBoard->LoadGame(aSaveName))
	{
		mFirstTimeGameSelector = false;
		if (mBoard->mLevelAwardSpawned)
			mBoardResult = BoardResult::BOARDRESULT_WON;
		KillDialog(Dialogs::DIALOG_ENDLESSSLOT);
		return true;
	}

	KillBoard();
	mEndlessSlotId = -1;
	return false;
}

void LawnApp::StartEndlessNewGame(GameMode theGameMode, int theSlot, const std::string& theName)
{
	mEndlessSlotId = theSlot;
	KillDialog(Dialogs::DIALOG_ENDLESSSLOT);
	KillChallengeScreen();

	InitEndlessSlotMeta(theGameMode, mPlayerInfo->mId, theSlot, theName);
	PreNewGame(theGameMode, false);
}

void LawnApp::DoConfirmDeleteEndlessSlot(int theSlot)
{
	(void)theSlot;
	LawnDialog* aDialog = (LawnDialog*)DoDialog(
		Dialogs::DIALOG_ENDLESSDELETE,
		true,
		"删除存档",
		"确定要删除这个存档吗？此操作不可恢复。",
		"[DIALOG_BUTTON_OK]",
		Dialog::BUTTONS_OK_CANCEL);
	aDialog->mLawnYesButton->mLabel = TodStringTranslate("[DIALOG_BUTTON_OK]");
}

void LawnApp::FinishConfirmDeleteEndlessSlot(bool isYes)
{
	KillDialog(Dialogs::DIALOG_ENDLESSDELETE);
	if (!isYes)
		return;

	EndlessSlotDialog* aSlotDialog = (EndlessSlotDialog*)GetDialog(Dialogs::DIALOG_ENDLESSSLOT);
	if (aSlotDialog)
		aSlotDialog->DeleteSelectedSlot();
}
```

> 说明：`DoConfirmDeleteEndlessSlot` 不直接使用 `theSlot`（删除动作由槽位对话框自己的 `mPendingDeleteSlot` 记住，`FinishConfirmDeleteEndlessSlot` 回调回去），故 `(void)theSlot;`。

- [ ] **Step 5: `ButtonDepress` 分发加 case**

2000 分支 switch 里加：

```cpp
		case Dialogs::DIALOG_ENDLESSSLOT:
			KillDialog(Dialogs::DIALOG_ENDLESSSLOT);
			return;

		case Dialogs::DIALOG_ENDLESSDELETE:
			FinishConfirmDeleteEndlessSlot(true);
			return;
```

3000 分支 switch 里加：

```cpp
		case Dialogs::DIALOG_ENDLESSDELETE:
			FinishConfirmDeleteEndlessSlot(false);
			return;
```

> 说明：`DIALOG_ENDLESSSLOT` 用 BUTTONS_FOOTER（只有取消按钮，id 走 2000 分支），点取消 = 关对话框留在挑战界面，故 3000 分支不需要它。

- [ ] **Step 6: `ChallengeScreen::ButtonDepress` 接入无尽入口**

替换 `src/Lawn/Widget/ChallengeScreen.cpp` 中（约 705-709 行）：

```cpp
	int aChallengeMode = theId - ChallengeScreen::ChallengeScreen_Mode;
	if (aChallengeMode >= 0 && aChallengeMode < NUM_CHALLENGE_MODES)
	{
		mApp->KillChallengeScreen();
		mApp->PreNewGame((GameMode)(aChallengeMode + 1), true);
	}
```

为：

```cpp
	int aChallengeMode = theId - ChallengeScreen::ChallengeScreen_Mode;
	if (aChallengeMode >= 0 && aChallengeMode < NUM_CHALLENGE_MODES)
	{
		GameMode aGameMode = (GameMode)(aChallengeMode + 1);
		if (mApp->IsSurvivalEndless(aGameMode) || mApp->IsEndlessIZombie(aGameMode) || mApp->IsEndlessScaryPotter(aGameMode))
		{
			mApp->DoEndlessSlotDialog(aGameMode);
		}
		else
		{
			mApp->KillChallengeScreen();
			mApp->PreNewGame(aGameMode, true);
		}
	}
```

> 说明：槽位对话框是覆盖在挑战界面上的模态层，先不杀挑战界面；开始/继续成功时 `LoadEndlessSlot`/`StartEndlessNewGame` 会调 `KillChallengeScreen()`。取消则对话框关闭、留在挑战界面。`IsSurvivalEndless` 等是 `LawnApp` 静态成员，经 `mApp->` 调用即可。

- [ ] **Step 7: 编译检查**

Run: `cmake --build build`
Expected: 编译通过。若报 `Layout`/`LayoutFlags` 未定义，在 EndlessSlotDialog.cpp 加 `#include "../../SexyAppFramework/widget/Widget.h"`；若报 `LENGTH` 未定义，加 `#include "../../SexyAppFramework/Common.h"`（已加）。

- [ ] **Step 8: 手动冒烟**

Run: `./run-pvz.sh`，操作：
1. 挑战模式 → 生存页 → 点泳池无尽 → 预期弹出"选择存档"对话框，5 行全部"空槽位"。
2. 点"开始/继续" → 弹命名框（默认"…存档1"）→ 回车 → 进入关卡。
3. 开始游戏后暂停 → 暂停菜单 → 返回主菜单。
4. 再进挑战 → 泳池无尽 → 对话框应显示"…存档1 — 已过 0 旗 · 第 0 波"，选中并"开始/继续" → 直接进入该局。
5. 对话框里点"取消" → 回到挑战界面。
Expected: 全部符合预期。

- [ ] **Step 9: 提交**

```bash
git add src/Lawn/Widget/EndlessSlotDialog.h src/Lawn/Widget/EndlessSlotDialog.cpp src/LawnApp.h src/LawnApp.cpp src/Lawn/Widget/ChallengeScreen.cpp
git commit -m "feat(endless-save): add EndlessSlotDialog and wire endless challenge entries to slot picker"
```

---

### Task 5: 全量手动测试矩阵

**Files:** 无（仅验证与修 bug）

- [ ] **Step 1: 编译并启动**

Run: `cmake --build build && ./run-pvz.sh`
Expected: 构建通过，游戏启动。

- [ ] **Step 2: 生存无尽（白天/夜晚/泳池/浓雾/屋顶 全 5 阶段）**

对每个阶段重复：
1. 开 2-3 个槽（命名不同名字），互相切换读取，确认进度各自独立（每个槽的旗帜数/波数不串）。
2. 打到旗帜波结束（自动存档点）→ 强杀进程（任务管理器/`kill`）→ 重进 → 该槽进度保留且 `已过 N 旗` 显示正确。
3. 暂停 → 暂停菜单存档 → 返回主菜单 → 重进 → 继续该槽。
4. 失败 → 回挑战界面 → 该槽变"空槽位"（.v4 和 .meta 都删）。
5. 暂停菜单"重启本关" → 同槽重开，名字保留、进度清零（显示"已过 0 旗"）。
Expected: 全部符合上述预期。

- [ ] **Step 3: 谜题无尽（我是僵尸无尽、惊吓盒无尽）**

对两个模式各：
1. 开 2 个槽，进入后完成 1 个阶段（吃脑/开盒），暂停存档 → 退出重进 → 显示"第 N 阶段"正确。
2. 失败清槽、改名、删除（含确认对话框）各验证一遍。
Expected: 符合预期。

- [ ] **Step 4: 改名与删除**

1. 槽位对话框选占用槽 → "改名" → 输入新名 → 回车 → 列表立即刷新显示新名。
2. 再选该槽 → "删除" → 确认框 → 确定 → 槽变"空槽位"，`.v4`/`.meta` 都被删（可在 `userdata/` 下核对文件名 `game{pid}_{mode}_s{slot}.v4/.meta`）。
Expected: 符合预期。

- [ ] **Step 5: 旧单槽存档迁移**

1. 退出游戏，手工构造旧存档：把某个无尽模式的 `game{pid}_{mode}.v4` 复制为 `userdata/game{pid}_{mode}.v4`（无 `_s` 后缀；可从任意现有 .v4 改名，或用 `scripts/pvzp-v4-converter.py` 生成合法文件）。
2. 启动游戏进该无尽模式 → 槽位对话框自动迁移：槽位 1 显示"有存档"（默认名），槽 1 的 `.v4` 可正常继续。
3. 核对旧文件已被删除。
Expected: 符合预期。

- [ ] **Step 6: 存档完整性**

用 `python scripts/pvzp-v4-converter.py info userdata/game{pid}_{mode}_s0.v4` 验证槽位存档可被解析（TLV 结构完整）。
Expected: info 输出正常。

- [ ] **Step 7: 非无尽模式回归**

1. 冒险模式：正常玩一关 → 暂停 → 退出 → 主菜单"继续"仍走原单槽流程。
2. 普通挑战（如"坚果保龄球"）、普通谜题（我是僵尸 1、惊吓盒 1）：点击后**不**弹槽位对话框，直接走原"读档/新局"流程。
3. 原 `userdata/game{pid}_{mode}.v4`（非无尽模式）不受影响。
Expected: 无回归。

- [ ] **Step 8: 收尾提交**

若有修复，逐个提交；最终 `git log --oneline` 应包含 Task 1-4 的 4 个 commit。

---

## Self-Review 记录

- **Spec 覆盖**：5 槽位/模式 ✓（Task 1 常量、Task 4 对话框 5 行）；命名+改名 ✓（Task 3+4）；删除确认 ✓（Task 4）；进度显示（旗/波/阶段）✓（Task 1 meta + Task 4 标签）；自动存档路由 ✓（Task 2 Board.cpp）；失败清槽 / RESTART 保留名字 ✓（Task 2 KillBoard）；旧档迁移 ✓（Task 1 MigrateLegacyEndlessSave + Task 4 打开对话框时调用）；.meta 缺失回退 ✓（Task 4 GetSlotLabel）；.v4 损坏提示 ✓（Task 4 读取失败 DoDialog）；非无尽回归 ✓（Task 4 ChallengeScreen 分支 + Task 5 Step 7）。
- **设计偏差**：CHEAT 与 RESTART 同处理（只删 .v4 保留名字同槽重开），因 `FinishCheatDialog` 不经 `KillBoard`，已在 Task 2 说明。
- **占位符扫描**：无 TBD/TODO；所有代码步骤含完整代码。
- **类型一致性**：`mEndlessSlotId`（int，-1 默认）在 Task 2 定义、Task 2/4 读写一致；`LoadEndlessSlot` 返回 bool、`StartEndlessNewGame` 返回 void、`CommitName(const std::string&)`、`DeleteSelectedSlot()` 在 Task 4 各调用点签名一致；`GetEndlessSaveName(mode, pid, slot)` 参数顺序全篇一致。
