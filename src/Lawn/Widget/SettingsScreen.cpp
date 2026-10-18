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

#include "SettingsScreen.h"
#include "GameButton.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "../../ConstEnums.h"
#include "../LawnCommon.h"
#include "../System/PlayerInfo.h"
#include "../../Sexy.TodLib/TodCommon.h"
#include "../../Sexy.TodLib/TodStringFile.h"
#include "widget/Checkbox.h"
#include "widget/WidgetManager.h"
#include "graphics/Font.h"

// +Gaga: 设置页布局（800x600 逻辑坐标）
static const int SETTINGS_ROW_X = 100;
static const int SETTINGS_ROW_Y = 120;
static const int SETTINGS_ROW_STEP = 40;
static const int SETTINGS_ROW_HEIGHT = 45;
static const int SETTINGS_LABEL_X = 160;
static const int SETTINGS_CHECKBOX_SIZE = 46;

static const int SETTINGS_COMPLETE_X = 520;
static const int SETTINGS_COMPLETE_Y = 200;
static const int SETTINGS_COMPLETE_WIDTH = 220;
static const int SETTINGS_COMPLETE_HEIGHT = 33;

static const int SETTINGS_BACK_X = 340;
static const int SETTINGS_BACK_Y = 520;
static const int SETTINGS_BACK_WIDTH = 120;
static const int SETTINGS_BACK_HEIGHT = 33;

static const char* SETTINGS_TITLE = "设置";
static const char* SETTINGS_PREREQUISITE_HINT = "皮纳塔 / 舞王 / 雏菊 分别需要智慧树分数达到 1000 / 500 / 100。";
static const char* SETTINGS_COMPLETE_HINT = "解锁小游戏 / 解谜 / 生存模式，标记冒险模式通关。";
static const char* SETTINGS_ONE_CLICK_HEADER = "一键通关冒险模式";
static const char* SETTINGS_ONE_CLICK_BODY =
	"是否真的要一键通关冒险模式？\n"
	"将解锁小游戏、解谜与生存模式，获得 50000 金币，\n"
	"所有挑战记录设为 20，并清空冒险模式的当前存档。\n"
	"此操作无法撤销。";

// 与 LawnApp::GagaCheatMode 一一对应，顺序不可随意调整
struct SettingsModeRow
{
	int				mId;
	GagaCheatMode	mMode;
	const char*		mLabel;
};

static const SettingsModeRow gSettingsModeRows[SettingsScreen::SETTINGS_MODE_COUNT] =
{
	{ SettingsScreen::SettingsScreen_Mustache,		GagaCheatMode::GAGA_CHEAT_MUSTACHE,		"胡子模式" },
	{ SettingsScreen::SettingsScreen_SuperMower,	GagaCheatMode::GAGA_CHEAT_SUPER_MOWER,	"超级割草机" },
	{ SettingsScreen::SettingsScreen_Future,		GagaCheatMode::GAGA_CHEAT_FUTURE,		"未来模式" },
	{ SettingsScreen::SettingsScreen_Pinata,		GagaCheatMode::GAGA_CHEAT_PINATA,		"皮纳塔模式" },
	{ SettingsScreen::SettingsScreen_Dance,			GagaCheatMode::GAGA_CHEAT_DANCE,		"舞王模式" },
	{ SettingsScreen::SettingsScreen_Daisy,			GagaCheatMode::GAGA_CHEAT_DAISY,		"雏菊模式" },
	{ SettingsScreen::SettingsScreen_Sukhbir,		GagaCheatMode::GAGA_CHEAT_SUKHBIR,		"Sukhbir 模式" }
};

// 该模式当前是否满足前置条件（与作弊键的判定完全一致）
static bool IsCheatModeAvailable(LawnApp* theApp, GagaCheatMode theMode)
{
	switch (theMode)
	{
	case GagaCheatMode::GAGA_CHEAT_PINATA:	return theApp->CanDoPinataMode();
	case GagaCheatMode::GAGA_CHEAT_DANCE:	return theApp->CanDoDanceMode();
	case GagaCheatMode::GAGA_CHEAT_DAISY:	return theApp->CanDoDaisyMode();
	default:								return true;
	}
}

static int GetModeRowIndex(int theId)
{
	for (int i = 0; i < SettingsScreen::SETTINGS_MODE_COUNT; i++)
	{
		if (gSettingsModeRows[i].mId == theId)
			return i;
	}

	return -1;
}

// 挑战背景比较花，文字统一先画一遍黑色描边再画本色
static void DrawShadowedString(Graphics* g, const std::string& theText, int theX, int theY, _Font* theFont, const Color& theColor, DrawStringJustification theJustification)
{
	TodDrawString(g, theText, theX + 1, theY + 1, theFont, Color(0, 0, 0), theJustification);
	TodDrawString(g, theText, theX, theY, theFont, theColor, theJustification);
}

SettingsScreen::SettingsScreen(LawnApp* theApp) :
	Dialog(nullptr, nullptr, Dialogs::DIALOG_SETTINGS, true, "Settings", "", "", Dialog::BUTTONS_NONE)
{
	mApp = theApp;
	mClip = false;
	mCompleteAdventureButton = nullptr;
	mBackButton = nullptr;
	for (int i = 0; i < SETTINGS_MODE_COUNT; i++)
		mModeCheckbox[i] = nullptr;

	// 背景图（Challenge_Background.jpg）在 DelayLoad_ChallengeScreen 组里，按需加载
	mLoadedResourceNames.push_back("DelayLoad_ChallengeScreen");
	for (std::string& aResource : mLoadedResourceNames)
		TodLoadResources(aResource.c_str());

	for (int i = 0; i < SETTINGS_MODE_COUNT; i++)
	{
		mModeCheckbox[i] = MakeNewCheckbox(gSettingsModeRows[i].mId, this, mApp->GetCheatMode(gSettingsModeRows[i].mMode));
	}

	mCompleteAdventureButton = MakeButton(SettingsScreen_CompleteAdventure, this, SETTINGS_ONE_CLICK_HEADER);
	mBackButton = MakeButton(SettingsScreen_Back, this, "返回");

	// 没有用户档案时无法一键通关
	if (mApp->mPlayerInfo == nullptr)
		mCompleteAdventureButton->SetDisabled(true);

	Resize(0, 0, theApp->mWidth, theApp->mHeight);
}

SettingsScreen::~SettingsScreen()
{
	for (int i = 0; i < SETTINGS_MODE_COUNT; i++)
	{
		if (mModeCheckbox[i])
			delete mModeCheckbox[i];
	}
	if (mCompleteAdventureButton)
		delete mCompleteAdventureButton;
	if (mBackButton)
		delete mBackButton;
}

void SettingsScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);

	for (int i = 0; i < SETTINGS_MODE_COUNT; i++)
	{
		if (mModeCheckbox[i])
			mModeCheckbox[i]->Resize(SETTINGS_ROW_X, SETTINGS_ROW_Y + SETTINGS_ROW_STEP * i, SETTINGS_CHECKBOX_SIZE, SETTINGS_ROW_HEIGHT);
	}

	if (mCompleteAdventureButton)
		mCompleteAdventureButton->Resize(SETTINGS_COMPLETE_X, SETTINGS_COMPLETE_Y, SETTINGS_COMPLETE_WIDTH, SETTINGS_COMPLETE_HEIGHT);
	if (mBackButton)
		mBackButton->Resize(SETTINGS_BACK_X, SETTINGS_BACK_Y, SETTINGS_BACK_WIDTH, SETTINGS_BACK_HEIGHT);
}

void SettingsScreen::Draw(Graphics* g)
{
	g->SetLinearBlend(true);
	g->DrawImage(IMAGE_CHALLENGE_BACKGROUND, 0, 0);

	// 半透明压暗背板，保证文字在挑战背景上依然可读
	g->SetColor(Color(0, 0, 0, 120));
	g->FillRect(60, 92, 680, 470);
	g->SetColor(Color::White);

	DrawShadowedString(g, SETTINGS_TITLE, 400, 58, FONT_HOUSEOFTERROR28, Color(220, 220, 220), DS_ALIGN_CENTER);

	for (int i = 0; i < SETTINGS_MODE_COUNT; i++)
	{
		bool aAvailable = IsCheatModeAvailable(mApp, gSettingsModeRows[i].mMode);
		std::string aLabel = gSettingsModeRows[i].mLabel;
		if (!aAvailable)
			aLabel += "（未解锁）";

		Color aColor = aAvailable ? Color(255, 255, 255) : Color(150, 150, 150);
		DrawShadowedString(g, aLabel, SETTINGS_LABEL_X, SETTINGS_ROW_Y + SETTINGS_ROW_STEP * i + 23, FONT_DWARVENTODCRAFT18, aColor, DS_ALIGN_LEFT);
	}

	DrawShadowedString(g, SETTINGS_PREREQUISITE_HINT, SETTINGS_ROW_X, 430, FONT_CONTINUUMBOLD14, Color(200, 200, 200), DS_ALIGN_LEFT);
	TodDrawStringWrapped(g, SETTINGS_COMPLETE_HINT, Rect(500, 244, 260, 80), FONT_CONTINUUMBOLD14, Color(220, 220, 220), DS_ALIGN_LEFT);
}

void SettingsScreen::Update()
{
	Dialog::Update();
	MarkDirty();
}

void SettingsScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);

	for (int i = 0; i < SETTINGS_MODE_COUNT; i++)
		AddWidget(mModeCheckbox[i]);
	AddWidget(mCompleteAdventureButton);
	AddWidget(mBackButton);
}

void SettingsScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);

	for (int i = 0; i < SETTINGS_MODE_COUNT; i++)
		RemoveWidget(mModeCheckbox[i]);
	RemoveWidget(mCompleteAdventureButton);
	RemoveWidget(mBackButton);
}

void SettingsScreen::OrderInManagerChanged()
{
	Dialog::OrderInManagerChanged();

	for (int i = 0; i < SETTINGS_MODE_COUNT; i++)
		mWidgetManager->PutInfront(mModeCheckbox[i], this);
	mWidgetManager->PutInfront(mCompleteAdventureButton, this);
	mWidgetManager->PutInfront(mBackButton, this);
}

void SettingsScreen::CheckboxChecked(int theId, bool checked)
{
	int aIndex = GetModeRowIndex(theId);
	if (aIndex < 0)
		return;

	if (!mApp->SetCheatMode(gSettingsModeRows[aIndex].mMode, checked))
	{
		// 前置条件不满足：和作弊键一样响一声，并把复选框拨回去
		mApp->PlaySample(SOUND_BUZZER);
		if (mModeCheckbox[aIndex])
			mModeCheckbox[aIndex]->SetChecked(!checked, false);
	}
}

void SettingsScreen::ButtonPress(int theId)
{
	(void)theId;
	mApp->PlaySample(SOUND_GRAVEBUTTON);
}

void SettingsScreen::ButtonDepress(int theId)
{
	switch (theId)
	{
	case SettingsScreen::SettingsScreen_Back:
		mApp->KillSettingsScreen();
		break;

	case SettingsScreen::SettingsScreen_CompleteAdventure:
	{
		if (mApp->mPlayerInfo == nullptr)
			break;

		int aResult = mApp->LawnMessageBox(
			Dialogs::DIALOG_MESSAGE,
			SETTINGS_ONE_CLICK_HEADER,
			SETTINGS_ONE_CLICK_BODY,
			"[DIALOG_BUTTON_YES]",
			"[DIALOG_BUTTON_NO]",
			Dialog::BUTTONS_YES_NO
		);

		if (aResult == Dialog::ID_YES)
		{
			// 先关掉设置页（解除模态），再执行一键通关并刷新主菜单
			mApp->KillSettingsScreen();
			mApp->CompleteAdventureMode();
		}
		break;
	}
	}
}

void SettingsScreen::KeyDown(KeyCode theKey)
{
	if (theKey == KeyCode::KEYCODE_ESCAPE)
	{
		mApp->KillSettingsScreen();
	}
}
