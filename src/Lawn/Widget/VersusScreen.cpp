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

#include "VersusScreen.h"
#include "GameButton.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "../../ConstEnums.h"
#include "../LawnCommon.h"
#include "../Plant.h"
#include "../Zombie.h"
#include "../SeedPacket.h"
#include "../System/PlayerInfo.h"
#include "../Versus.h"
#include "../../Sexy.TodLib/TodCommon.h"
#include "../../Sexy.TodLib/TodStringFile.h"
#include "widget/EditWidget.h"
#include "widget/WidgetManager.h"
#include "graphics/Font.h"

static const int VERSUS_CARD_COLS = 10;
static const int VERSUS_CARD_STEP_X = 66;
static const int VERSUS_CARD_STEP_Y = 64;

static void VersusShadowString(Graphics* g, const std::string& theText, int theX, int theY, _Font* theFont, const Color& theColor, DrawStringJustification theJustification = DS_ALIGN_LEFT)
{
	TodDrawString(g, theText, theX + 1, theY + 1, theFont, Color(0, 0, 0), theJustification);
	TodDrawString(g, theText, theX, theY, theFont, theColor, theJustification);
}

VersusScreen::VersusScreen(LawnApp* theApp) :
	Dialog(nullptr, nullptr, Dialogs::DIALOG_VERSUS, true, "本地联机对战", "", "", Dialog::BUTTONS_NONE)
{
	mApp = theApp;
	mSession = theApp->mVersusSession;
	mState = ScreenState::Menu;
	mClip = false;
	mPlantCount = 0;
	mZombieCount = 0;
	mImitaterPicking = false;
	mImitaterSlot = -1;
	mHoverCard = -1;
	mLocalReadySent = false;
	mPickedSide = VersusProtocol::SIDE_NONE;
	for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
	{
		mPlantSeeds[i] = -1;
		mPlantImitater[i] = -1;
		mZombieSeeds[i] = -1;
	}

	mLoadedResourceNames.push_back("DelayLoad_ChallengeScreen");
	for (std::string& aResource : mLoadedResourceNames)
		TodLoadResources(aResource.c_str());

	// 选卡界面会显示全部植物/僵尸卡面，先预加载对应资源。
	for (int i = 0; i < 49; i++)
	{
		Plant::PreloadPlantResources(static_cast<SeedType>(i));
	}
	for (int i = 0; i < gVersusZombieCardCount; i++)
	{
		Zombie::PreloadZombieResources(static_cast<ZombieType>(gVersusZombieCards[i].mZombieType));
	}

	mBackButton = MakeButton(VersusScreen_Back, this, "返回");
	mHostButton = MakeButton(VersusScreen_Host, this, "创建房间");
	mJoinButton = MakeButton(VersusScreen_Join, this, "加入房间");
	mAddressEdit = CreateEditWidget(0, this, this);
	mAddressEdit->SetText("127.0.0.1", false);
	mPlantButton = MakeButton(VersusScreen_Plant, this, "植物方");
	mZombieButton = MakeButton(VersusScreen_Zombie, this, "僵尸方");
	mCancelSideButton = MakeButton(VersusScreen_CancelSide, this, "取消选择");
	mResetSidesButton = MakeButton(VersusScreen_ResetSides, this, "重置选边");
	mPickButton = MakeButton(VersusScreen_Pick, this, "选卡");
	mReadyButton = MakeButton(VersusScreen_Ready, this, "准备");
	mStartButton = MakeButton(VersusScreen_Start, this, "开始");

	Resize(0, 0, theApp->mWidth, theApp->mHeight);
}

VersusScreen::~VersusScreen()
{
	delete mBackButton;
	delete mHostButton;
	delete mJoinButton;
	delete mAddressEdit;
	delete mPlantButton;
	delete mZombieButton;
	delete mCancelSideButton;
	delete mResetSidesButton;
	delete mPickButton;
	delete mReadyButton;
	delete mStartButton;
}

void VersusScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	mBackButton->Resize(30, 548, 120, 33);
	mHostButton->Resize(130, 245, 220, 40);
	mJoinButton->Resize(450, 245, 220, 40);
	mAddressEdit->Resize(450, 300, 220, 32);
	mPlantButton->Resize(170, 220, 200, 40);
	mZombieButton->Resize(430, 220, 200, 40);
	mCancelSideButton->Resize(230, 285, 150, 33);
	mResetSidesButton->Resize(420, 285, 150, 33);
	mPickButton->Resize(250, 350, 140, 40);
	mReadyButton->Resize(640, 548, 130, 33);
	mStartButton->Resize(640, 470, 130, 33);
}

void VersusScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	AddWidget(mBackButton);
	AddWidget(mHostButton);
	AddWidget(mJoinButton);
	AddWidget(mAddressEdit);
	AddWidget(mPlantButton);
	AddWidget(mZombieButton);
	AddWidget(mCancelSideButton);
	AddWidget(mResetSidesButton);
	AddWidget(mPickButton);
	AddWidget(mReadyButton);
	AddWidget(mStartButton);
	UpdateButtonVisibility();
}

void VersusScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	RemoveWidget(mBackButton);
	RemoveWidget(mHostButton);
	RemoveWidget(mJoinButton);
	RemoveWidget(mAddressEdit);
	RemoveWidget(mPlantButton);
	RemoveWidget(mZombieButton);
	RemoveWidget(mCancelSideButton);
	RemoveWidget(mResetSidesButton);
	RemoveWidget(mPickButton);
	RemoveWidget(mReadyButton);
	RemoveWidget(mStartButton);
}

void VersusScreen::UpdateButtonVisibility()
{
	bool aMenu = mState == ScreenState::Menu && mSession->GetPhase() != VersusSession::Phase::Lobby;
	bool aLobby = mState == ScreenState::Lobby || mSession->GetPhase() == VersusSession::Phase::Lobby;
	bool aPicking = mState == ScreenState::Picking;
	bool aConnected = mSession->GetNetwork().IsConnected();
	bool aCanConnect = mSession->GetPhase() == VersusSession::Phase::Idle || mSession->GetPhase() == VersusSession::Phase::Error;

	mHostButton->mVisible = aMenu && aCanConnect;
	mJoinButton->mVisible = aMenu && aCanConnect;
	mAddressEdit->mVisible = aMenu && aCanConnect;
	mPlantButton->mVisible = aLobby && !aPicking;
	mZombieButton->mVisible = aLobby && !aPicking;
	mCancelSideButton->mVisible = aLobby && !aPicking;
	mResetSidesButton->mVisible = aLobby && !aPicking && mSession->IsHost();
	mPickButton->mVisible = aLobby && !aPicking;
	mReadyButton->mVisible = aPicking;
	mStartButton->mVisible = !aMenu && mSession->IsHost();

	mHostButton->SetDisabled(aConnected);
	mJoinButton->SetDisabled(aConnected);
	mPlantButton->SetDisabled(mSession->GetRemoteSide() == VersusProtocol::SIDE_PLANT);
	mZombieButton->SetDisabled(mSession->GetRemoteSide() == VersusProtocol::SIDE_ZOMBIE);
	mCancelSideButton->SetDisabled(mSession->GetLocalSide() == VersusProtocol::SIDE_NONE);
	mPickButton->SetDisabled(mSession->GetLocalSide() == VersusProtocol::SIDE_NONE);
	mStartButton->SetDisabled(!mSession->CanStartMatch());
}

void VersusScreen::Update()
{
	Dialog::Update();
	MarkDirty();

	if (mSession == nullptr)
	{
		return;
	}
	mSession->Update();

	uint8_t aLocalSide = mSession->GetLocalSide();
	if (aLocalSide != mPickedSide)
	{
		ResetPicks();
		mPickedSide = aLocalSide;
	}

	if (mSession->TakeStartRequest())
	{
		mApp->StartVersusMatch(mSession->GetConfig());
		return;
	}

	if (mSession->GetPhase() == VersusSession::Phase::Lobby && mState == ScreenState::Menu)
	{
		mState = ScreenState::Lobby;
	}
	if (mSession->GetPhase() == VersusSession::Phase::Error)
	{
		mState = ScreenState::Menu;
	}

	if (mState == ScreenState::Picking)
	{
		bool aReady = mSession->IsLocalReady();
		mReadyButton->SetLabel(aReady ? "取消准备" : "准备");
	}
	UpdateButtonVisibility();
}

void VersusScreen::Draw(Graphics* g)
{
	g->SetLinearBlend(true);
	if (Sexy::IMAGE_CHALLENGE_BACKGROUND)
	{
		g->DrawImage(Sexy::IMAGE_CHALLENGE_BACKGROUND, 0, 0);
	}
	else
	{
		g->SetColor(Color(20, 40, 20));
		g->FillRect(0, 0, mWidth, mHeight);
	}

	VersusShadowString(g, "本地联机对战", 400, 55, FONT_DWARVENTODCRAFT18YELLOW, Color(255, 240, 120), DS_ALIGN_CENTER);
	VersusShadowString(g, mSession->GetStatus(), 400, 88, FONT_BRIANNETOD12, Color::White, DS_ALIGN_CENTER);

	if (mState == ScreenState::Menu)
	{
		DrawMenu(g);
	}
	else if (mState == ScreenState::Lobby)
	{
		DrawLobby(g);
	}
	else
	{
		DrawPicker(g);
	}
}

void VersusScreen::DrawMenu(Graphics* g)
{
	VersusShadowString(g, "一台电脑创建房间，另一台输入主机 IP 加入", 400, 180, FONT_BRIANNETOD12, Color(230, 230, 230), DS_ALIGN_CENTER);
	VersusShadowString(g, "主机 IP: " + VersusNetwork::GetLocalIPv4() + "  端口: " + std::to_string(VERSUS_PORT), 400, 205, FONT_BRIANNETOD12, Color(255, 220, 120), DS_ALIGN_CENTER);
	VersusShadowString(g, "同一台电脑测试可输入 127.0.0.1", 400, 340, FONT_BRIANNETOD12, Color(180, 220, 255), DS_ALIGN_CENTER);
	if (mSession->GetPhase() == VersusSession::Phase::Error)
	{
		VersusShadowString(g, mSession->GetStatus(), 400, 375, FONT_BRIANNETOD12, Color(255, 120, 120), DS_ALIGN_CENTER);
	}
}

void VersusScreen::DrawLobby(Graphics* g)
{
	std::string aLocal = mSession->GetLocalSide() == VersusProtocol::SIDE_PLANT ? "植物方" : mSession->GetLocalSide() == VersusProtocol::SIDE_ZOMBIE ? "僵尸方" : "未选择";
	std::string aRemote = mSession->GetRemoteSide() == VersusProtocol::SIDE_PLANT ? "植物方" : mSession->GetRemoteSide() == VersusProtocol::SIDE_ZOMBIE ? "僵尸方" : "未选择";
	VersusShadowString(g, "你: " + aLocal + "    对手: " + aRemote, 400, 180, FONT_BRIANNETOD12, Color(255, 240, 150), DS_ALIGN_CENTER);
	VersusShadowString(g, "双方各自点选阵营；已被占用的阵营会置灰。房主可重置选边。", 400, 500, FONT_BRIANNETOD12, Color(220, 220, 220), DS_ALIGN_CENTER);
	VersusShadowString(g, std::string("本机准备: ") + (mSession->IsLocalReady() ? "是" : "否") + "    对手准备: " + (mSession->IsRemoteReady() ? "是" : "否"), 400, 525, FONT_BRIANNETOD12, Color(220, 220, 220), DS_ALIGN_CENTER);
}

void VersusScreen::DrawPicker(Graphics* g)
{
	bool aZombie = mSession->GetLocalSide() == VersusProtocol::SIDE_ZOMBIE;
	if (mImitaterPicking)
	{
		aZombie = false;
	}
	VersusShadowString(g, aZombie ? "选择 6 张僵尸卡（脑光价格）" : "选择 6 张植物卡（阳光价格）", 400, 78, FONT_BRIANNETOD12, Color(255, 240, 150), DS_ALIGN_CENTER);
	DrawCardGrid(g, aZombie);
	DrawSelectedSlots(g, aZombie);
}

void VersusScreen::CardGridPosition(int theIndex, int& x, int& y) const
{
	x = 58 + (theIndex % VERSUS_CARD_COLS) * VERSUS_CARD_STEP_X;
	y = 112 + (theIndex / VERSUS_CARD_COLS) * VERSUS_CARD_STEP_Y;
}

static void VersusDrawZombieCard(Graphics* g, int x, int y, int theSeed, bool theSelected)
{
	// 僵尸卡不是普通 packet 图，直接 DrawSeedPacket 会把整只僵尸按原尺寸画出来并互相重叠。
	g->SetColor(Color::White);
	TodDrawImageCelScaledF(g, Sexy::IMAGE_SEEDS, x, y, 2, 0, g->mScaleX, g->mScaleY);
	if (theSelected)
	{
		g->SetColor(Color(60, 60, 60, 150));
		g->FillRect(x, y, 50, 70);
	}
	g->SetColor(Color::White);
	SeedPacketDrawSeed(g, x, y, static_cast<SeedType>(theSeed), SeedType::SEED_NONE, 4.0f, 8.0f, 0.22f);
	g->SetColor(Color::White);
}

void VersusScreen::DrawCardGrid(Graphics* g, bool theZombie)
{
	int aCount = theZombie ? gVersusZombieCardCount : 49;
	for (int i = 0; i < aCount; i++)
	{
		int x, y;
		CardGridPosition(i, x, y);
		SeedType aSeed = theZombie
			? static_cast<SeedType>(gVersusZombieCards[i].mSeedType)
			: static_cast<SeedType>(i);
		int aCost = theZombie ? gVersusZombieCards[i].mCost : Plant::GetCost(aSeed, SeedType::SEED_NONE);
		bool aSelected = false;
		int aCountSelected = theZombie ? mZombieCount : mPlantCount;
		for (int j = 0; j < aCountSelected; j++)
		{
			if ((theZombie ? mZombieSeeds[j] : (mPlantSeeds[j] == SeedType::SEED_IMITATER ? mPlantImitater[j] : mPlantSeeds[j])) == aSeed)
			{
				aSelected = true;
				break;
			}
			if (theZombie && mZombieSeeds[j] == aSeed)
			{
				aSelected = true;
				break;
			}
		}
		if (mHoverCard == i)
		{
			g->SetColor(Color(255, 255, 255, 60));
			g->FillRect(x - 2, y - 2, 54, 74);
		}
		if (theZombie)
			VersusDrawZombieCard(g, x, y, aSeed, aSelected);
		else
			DrawSeedPacket(g, x, y, aSeed, SeedType::SEED_NONE, 0.0f, aSelected ? 100 : 255, false, false);
		std::string aCostText = std::to_string(aCost);
		VersusShadowString(g, aCostText, x + 25, y + 68, FONT_CONTINUUMBOLD14, theZombie ? Color(255, 150, 220) : Color(255, 240, 120), DS_ALIGN_CENTER);
	}
}

void VersusScreen::DrawSelectedSlots(Graphics* g, bool theZombie)
{
	int aCount = theZombie ? mZombieCount : mPlantCount;
	for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
	{
		int x = 175 + i * 66;
		int y = 500;
		g->SetColor(Color(30, 30, 30, 180));
		g->FillRect(x - 2, y - 2, 54, 74);
		g->SetColor(Color(120, 120, 120));
		g->DrawRect(x - 2, y - 2, 54, 74);
		if (i < aCount)
		{
			int aSeed = theZombie ? mZombieSeeds[i] : mPlantSeeds[i];
			int aImitater = theZombie ? -1 : mPlantImitater[i];
			if (theZombie)
				VersusDrawZombieCard(g, x, y, aSeed, false);
			else
				DrawSeedPacket(g, x, y, static_cast<SeedType>(aSeed), static_cast<SeedType>(aImitater), 0.0f, 255, false, false);
		}
	}
}

bool VersusScreen::CardGridHitTest(int x, int y, bool theZombie, int& theIndex) const
{
	int aCount = theZombie ? gVersusZombieCardCount : 49;
	for (int i = 0; i < aCount; i++)
	{
		int cx, cy;
		CardGridPosition(i, cx, cy);
		if (x >= cx && x < cx + 50 && y >= cy && y < cy + 70)
		{
			theIndex = i;
			return true;
		}
	}
	theIndex = -1;
	return false;
}

void VersusScreen::AddSelectedCard(int theSeed, int theImitater)
{
	bool aZombie = mSession->GetLocalSide() == VersusProtocol::SIDE_ZOMBIE;
	if (aZombie)
	{
		if (mZombieCount >= VersusProtocol::CARD_SLOTS)
			return;
		for (int i = 0; i < mZombieCount; i++)
			if (mZombieSeeds[i] == theSeed)
				return;
		mZombieSeeds[mZombieCount++] = theSeed;
	}
	else
	{
		if (mPlantCount >= VersusProtocol::CARD_SLOTS)
			return;
		for (int i = 0; i < mPlantCount; i++)
			if (mPlantSeeds[i] == theSeed && mPlantImitater[i] == theImitater)
				return;
		mPlantSeeds[mPlantCount] = theSeed;
		mPlantImitater[mPlantCount] = theImitater;
		mPlantCount++;
	}
	SyncLoadoutToSession();
	if (mSession->IsLocalReady())
	{
		SendReady(false);
	}
}

void VersusScreen::RemoveSelectedSlot(int theSlot)
{
	if (theSlot < 0)
		return;
	bool aZombie = mSession->GetLocalSide() == VersusProtocol::SIDE_ZOMBIE;
	if (aZombie)
	{
		if (theSlot >= mZombieCount)
			return;
		for (int i = theSlot; i + 1 < mZombieCount; i++)
			mZombieSeeds[i] = mZombieSeeds[i + 1];
		mZombieSeeds[--mZombieCount] = -1;
	}
	else
	{
		if (theSlot >= mPlantCount)
			return;
		for (int i = theSlot; i + 1 < mPlantCount; i++)
		{
			mPlantSeeds[i] = mPlantSeeds[i + 1];
			mPlantImitater[i] = mPlantImitater[i + 1];
		}
		mPlantSeeds[--mPlantCount] = -1;
		mPlantImitater[mPlantCount] = -1;
	}
	SyncLoadoutToSession();
	if (mSession->IsLocalReady())
	{
		SendReady(false);
	}
}

void VersusScreen::ResetPicks()
{
	mPlantCount = 0;
	mZombieCount = 0;
	mImitaterPicking = false;
	for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
	{
		mPlantSeeds[i] = -1;
		mPlantImitater[i] = -1;
		mZombieSeeds[i] = -1;
	}
	mSession->SetLocalLoadout(mPlantSeeds, mPlantImitater, mZombieSeeds);
}

void VersusScreen::SyncLoadoutToSession()
{
	mSession->SetLocalLoadout(mPlantSeeds, mPlantImitater, mZombieSeeds);
}

void VersusScreen::SendReady(bool theReady)
{
	SyncLoadoutToSession();
	if (theReady && (mSession->GetLocalSide() == VersusProtocol::SIDE_PLANT ? mPlantCount : mZombieCount) != VersusProtocol::CARD_SLOTS)
	{
		mApp->PlaySample(SOUND_BUZZER);
		return;
	}
	mSession->SendPick(theReady);
	mLocalReadySent = theReady;
}

void VersusScreen::ButtonDepress(int theId)
{
	switch (theId)
	{
	case VersusScreen_Back:
		if (mSession)
			mSession->Close();
		mApp->KillVersusScreen();
		break;
	case VersusScreen_Host:
		if (!mSession->StartHost())
		{
			mApp->PlaySample(SOUND_BUZZER);
		}
		break;
	case VersusScreen_Join:
		if (!mSession->StartJoin(mAddressEdit->mString))
		{
			mApp->PlaySample(SOUND_BUZZER);
		}
		break;
	case VersusScreen_Plant:
		if (!mSession->SendSide(VersusProtocol::SIDE_PLANT))
			mApp->PlaySample(SOUND_BUZZER);
		break;
	case VersusScreen_Zombie:
		if (!mSession->SendSide(VersusProtocol::SIDE_ZOMBIE))
			mApp->PlaySample(SOUND_BUZZER);
		break;
	case VersusScreen_CancelSide:
		mSession->ReleaseLocalSide();
		ResetPicks();
		mPickedSide = VersusProtocol::SIDE_NONE;
		break;
	case VersusScreen_ResetSides:
		mSession->ResetSides();
		ResetPicks();
		mPickedSide = VersusProtocol::SIDE_NONE;
		break;
	case VersusScreen_Pick:
		mState = ScreenState::Picking;
		mImitaterPicking = false;
		break;
	case VersusScreen_Ready:
		SendReady(!mSession->IsLocalReady());
		break;
	case VersusScreen_Start:
		if (!mSession->StartMatch())
			mApp->PlaySample(SOUND_BUZZER);
		break;
	}
}

void VersusScreen::MouseDown(int x, int y, int theClickCount)
{
	Dialog::MouseDown(x, y, theClickCount);
	if (mState != ScreenState::Picking)
	{
		return;
	}

	// 点击底部卡槽移除
	bool aZombie = mSession->GetLocalSide() == VersusProtocol::SIDE_ZOMBIE;
	for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
	{
		int sx = 175 + i * 66;
		int sy = 500;
		if (x >= sx && x < sx + 50 && y >= sy && y < sy + 70)
		{
			RemoveSelectedSlot(i);
			return;
		}
	}

	int aIndex = -1;
	bool aPickZombie = mImitaterPicking ? false : aZombie;
	if (!CardGridHitTest(x, y, aPickZombie, aIndex))
	{
		return;
	}

	if (mImitaterPicking)
	{
		int aTarget = aIndex;
		mImitaterPicking = false;
		if (mPlantCount < VersusProtocol::CARD_SLOTS)
		{
			mPlantSeeds[mPlantCount] = SeedType::SEED_IMITATER;
			mPlantImitater[mPlantCount] = aTarget;
			mPlantCount++;
			SyncLoadoutToSession();
			if (mSession->IsLocalReady())
			{
				SendReady(false);
			}
		}
		return;
	}

	if (aZombie)
	{
		AddSelectedCard(gVersusZombieCards[aIndex].mSeedType, -1);
	}
	else
	{
		SeedType aSeed = static_cast<SeedType>(aIndex);
		if (aSeed == SeedType::SEED_IMITATER)
		{
			mImitaterPicking = true;
			return;
		}
		AddSelectedCard(aSeed, -1);
	}
}

void VersusScreen::MouseMove(int x, int y)
{
	Dialog::MouseMove(x, y);
	if (mState != ScreenState::Picking)
	{
		mHoverCard = -1;
		return;
	}
	bool aZombie = mSession->GetLocalSide() == VersusProtocol::SIDE_ZOMBIE && !mImitaterPicking;
	int aIndex = -1;
	if (CardGridHitTest(x, y, aZombie, aIndex))
		mHoverCard = aIndex;
	else
		mHoverCard = -1;
}

void VersusScreen::KeyDown(KeyCode theKey)
{
	if (theKey == KeyCode::KEYCODE_ESCAPE)
	{
		if (mState == ScreenState::Picking)
		{
			mState = ScreenState::Lobby;
			return;
		}
		if (mSession)
			mSession->Close();
		mApp->KillVersusScreen();
	}
}

void VersusScreen::EditWidgetText(int theId, const std::string& theString)
{
	(void)theId;
	(void)theString;
}
