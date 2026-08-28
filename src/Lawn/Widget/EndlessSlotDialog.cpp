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

#include "EndlessSlotDialog.h"
#include "GameButton.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "../../ConstEnums.h"
#include "../System/EndlessSlot.h"
#include "../System/PlayerInfo.h"
#include "../../SexyAppFramework/widget/DialogButton.h"
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

	mStartButton = MakeButton(EndlessSlotDialog_Start, this, "Start");
	mRenameButton = MakeButton(EndlessSlotDialog_Rename, this, "Rename");
	mDeleteButton = MakeButton(EndlessSlotDialog_Delete, this, "Delete");

	mTallBottom = true;
	CalcSize(210, 260);
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
	// 三个石质按钮堆叠在 footer（mLawnYesButton）上方，居中
	int aBtnX = mLawnYesButton->mX + (mLawnYesButton->mWidth - 160) / 2;
	int aStartY = mLawnYesButton->mY - 33 * 3 - 8;
	mStartButton->Resize(aBtnX, aStartY, 160, 33);
	mRenameButton->Resize(aBtnX, aStartY + 33 + 4, 160, 33);
	mDeleteButton->Resize(aBtnX, aStartY + (33 + 4) * 2, 160, 33);
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
