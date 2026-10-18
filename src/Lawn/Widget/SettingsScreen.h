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

#ifndef __SETTINGSSCREEN_H__
#define __SETTINGSSCREEN_H__

// +Gaga: 主菜单右下角「设置」按钮打开的整屏设置页。
// 背景为 Challenge_Background.jpg，左侧是 7 个可持久化的模式/彩蛋开关，
// 右侧是「一键通关冒险模式」按钮（带是/否确认框）。

#include "../../ConstEnums.h"
#include "widget/Dialog.h"
#include "widget/CheckboxListener.h"

class LawnApp;
class LawnStoneButton;

namespace Sexy
{
	class Checkbox;
}

using namespace Sexy;

class SettingsScreen : public Dialog, public CheckboxListener
{
public:
	enum
	{
		SettingsScreen_Back = 200,
		SettingsScreen_CompleteAdventure = 201,
		SettingsScreen_Mustache = 300,
		SettingsScreen_SuperMower,
		SettingsScreen_Future,
		SettingsScreen_Pinata,
		SettingsScreen_Dance,
		SettingsScreen_Daisy,
		SettingsScreen_Sukhbir
	};

	enum
	{
		SETTINGS_MODE_COUNT = 7
	};

public:
	LawnApp*			mApp;
	LawnStoneButton*	mCompleteAdventureButton;
	LawnStoneButton*	mBackButton;
	Checkbox*			mModeCheckbox[SETTINGS_MODE_COUNT];

public:
	SettingsScreen(LawnApp* theApp);
	virtual ~SettingsScreen();

	virtual void		Resize(int theX, int theY, int theWidth, int theHeight);
	virtual void		Draw(Graphics* g);
	virtual void		Update();
	virtual void		AddedToManager(WidgetManager* theWidgetManager);
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager);
	virtual void		OrderInManagerChanged();
	// 整屏页面不允许像普通对话框那样拖动
	virtual void		MouseDown(int x, int y, int theBtnNum, int theClickCount) { Widget::MouseDown(x, y, theBtnNum, theClickCount); }
	virtual void		MouseDrag(int x, int y) { (void)x; (void)y; }
	virtual void		CheckboxChecked(int theId, bool checked);
	virtual void		ButtonPress(int theId);
	virtual void		ButtonDepress(int theId);
	virtual void		KeyDown(KeyCode theKey);
};

#endif
