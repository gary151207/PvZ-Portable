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

#ifndef __VERSUSSCREEN_H__
#define __VERSUSSCREEN_H__

#include "../../ConstEnums.h"
#include "widget/Dialog.h"
#include "widget/EditListener.h"
#include "../System/VersusProtocol.h"

class LawnApp;
class LawnStoneButton;
class VersusSession;

namespace Sexy
{
	class EditWidget;
}

using namespace Sexy;

class VersusScreen : public Dialog, public EditListener
{
public:
	enum
	{
		VersusScreen_Back = 500,
		VersusScreen_Host,
		VersusScreen_Join,
		VersusScreen_Plant,
		VersusScreen_Zombie,
		VersusScreen_CancelSide,
		VersusScreen_ResetSides,
		VersusScreen_Pick,
		VersusScreen_Ready,
		VersusScreen_Start
	};

	enum class ScreenState
	{
		Menu,
		Lobby,
		Picking
	};

	LawnApp*			mApp;
	VersusSession*		mSession;
	ScreenState			mState;
	LawnStoneButton*	mBackButton;
	LawnStoneButton*	mHostButton;
	LawnStoneButton*	mJoinButton;
	EditWidget*			mAddressEdit;
	LawnStoneButton*	mPlantButton;
	LawnStoneButton*	mZombieButton;
	LawnStoneButton*	mCancelSideButton;
	LawnStoneButton*	mResetSidesButton;
	LawnStoneButton*	mPickButton;
	LawnStoneButton*	mReadyButton;
	LawnStoneButton*	mStartButton;
	int					mPlantSeeds[VersusProtocol::CARD_SLOTS];
	int					mPlantImitater[VersusProtocol::CARD_SLOTS];
	int					mZombieSeeds[VersusProtocol::CARD_SLOTS];
	int					mPlantCount;
	int					mZombieCount;
	bool				mImitaterPicking;
	int					mImitaterSlot;
	int					mHoverCard;
	bool				mLocalReadySent;
	uint8_t				mPickedSide;

public:
	VersusScreen(LawnApp* theApp);
	virtual ~VersusScreen();

	virtual void		Resize(int theX, int theY, int theWidth, int theHeight);
	virtual void		Draw(Graphics* g);
	virtual void		Update();
	virtual void		AddedToManager(WidgetManager* theWidgetManager);
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager);
	virtual void		ButtonDepress(int theId);
	virtual void		MouseDown(int x, int y, int theClickCount);
	virtual void		MouseMove(int x, int y);
	virtual void		KeyDown(KeyCode theKey);
	virtual void		EditWidgetText(int theId, const std::string& theString);

private:
	void				UpdateButtonVisibility();
	void				DrawMenu(Graphics* g);
	void				DrawLobby(Graphics* g);
	void				DrawPicker(Graphics* g);
	void				DrawCardGrid(Graphics* g, bool theZombie);
	void				DrawSelectedSlots(Graphics* g, bool theZombie);
	bool				CardGridHitTest(int x, int y, bool theZombie, int& theIndex) const;
	void				CardGridPosition(int theIndex, int& x, int& y) const;
	void				AddSelectedCard(int theSeed, int theImitater);
	void				RemoveSelectedSlot(int theSlot);
	void				SyncLoadoutToSession();
	void				SendReady(bool theReady);
	void				ResetPicks();
};

#endif
