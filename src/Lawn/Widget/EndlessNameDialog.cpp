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
