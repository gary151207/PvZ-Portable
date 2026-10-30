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

#include "Versus.h"
#include "Board.h"
#include "Plant.h"
#include "Zombie.h"
#include "Challenge.h"
#include "GridItem.h"
#include "Coin.h"
#include "SeedPacket.h"
#include "CursorObject.h"
#include "Widget/GameButton.h"
#include "widget/Dialog.h"
#include "../LawnApp.h"
#include "../Resources.h"
#include "System/PlayerInfo.h"
#include "../Sexy.TodLib/TodStringFile.h"
#include "../Sexy.TodLib/TodCommon.h"
#include "../Sexy.TodLib/Reanimator.h"
#include "graphics/Font.h"

#include <cstring>

// +Gaga: 联机对战僵尸卡池。前 15 张沿用 I, Zombie 价格，后 8 张按强度定价。
const VersusZombieCard gVersusZombieCards[] =
{
	{ SEED_ZOMBIE_NORMAL,            ZOMBIE_NORMAL,            50,  "普通僵尸",     false },
	{ SEED_ZOMBIE_TRAFFIC_CONE,      ZOMBIE_TRAFFIC_CONE,      75,  "路障僵尸",     false },
	{ SEED_ZOMBIE_POLEVAULTER,       ZOMBIE_POLEVAULTER,       75,  "撑杆僵尸",     false },
	{ SEED_ZOMBIE_PAIL,              ZOMBIE_PAIL,              125, "铁桶僵尸",     false },
	{ SEED_ZOMBIE_NEWSPAPER,         ZOMBIE_NEWSPAPER,         100, "读报僵尸",     false },
	{ SEED_ZOMBIE_SCREEN_DOOR,       ZOMBIE_DOOR,              100, "铁门僵尸",     false },
	{ SEED_ZOMBIE_FOOTBALL,          ZOMBIE_FOOTBALL,          175, "橄榄球僵尸",   false },
	{ SEED_ZOMBIE_DANCER,            ZOMBIE_DANCER,            350, "舞王僵尸",     false },
	{ SEED_ZOMBIE_SNORKEL,           ZOMBIE_SNORKEL,           100, "潜水僵尸",     true  },
	{ SEED_ZOMBIE_BOBSLED,           ZOMBIE_BOBSLED,           250, "雪橇车僵尸",   false },
	{ SEED_ZOMBIE_DOLPHIN_RIDER,     ZOMBIE_DOLPHIN_RIDER,     150, "海豚骑士僵尸", true  },
	{ SEED_ZOMBIE_JACK_IN_THE_BOX,   ZOMBIE_JACK_IN_THE_BOX,   125, "小丑僵尸",     false },
	{ SEED_ZOMBIE_BALLOON,           ZOMBIE_BALLOON,           150, "气球僵尸",     false },
	{ SEED_ZOMBIE_DIGGER,            ZOMBIE_DIGGER,            125, "矿工僵尸",     false },
	{ SEED_ZOMBIE_POGO,              ZOMBIE_POGO,              200, "跳跳僵尸",     false },
	{ SEED_ZOMBIE_YETI,              ZOMBIE_YETI,              200, "雪人僵尸",     false },
	{ SEED_ZOMBIE_BUNGEE,            ZOMBIE_BUNGEE,            125, "蹦极僵尸",     false },
	{ SEED_ZOMBIE_LADDER,            ZOMBIE_LADDER,            150, "梯子僵尸",     false },
	{ SEED_ZOMBIE_CATAPULT,          ZOMBIE_CATAPULT,          200, "投石车僵尸",   false },
	{ SEED_ZOMBIE_GARGANTUAR,        ZOMBIE_GARGANTUAR,        300, "巨人僵尸",     false },
	{ SEED_ZOMBIE_IMP,               ZOMBIE_IMP,               50,  "小鬼僵尸",     false },
	{ SEED_ZOMBIE_REDEYE_GARGANTUAR, ZOMBIE_REDEYE_GARGANTUAR, 400, "红眼巨人僵尸", false },
	{ SEED_ZOMBONI,                  ZOMBIE_ZAMBONI,           175, "冰车僵尸",     false }
};
const int gVersusZombieCardCount = static_cast<int>(sizeof(gVersusZombieCards) / sizeof(gVersusZombieCards[0]));

namespace
{
	uint32_t HashBytes(uint32_t theHash, const void* theData, size_t theSize)
	{
		const uint8_t* aBytes = static_cast<const uint8_t*>(theData);
		for (size_t i = 0; i < theSize; i++)
		{
			theHash ^= aBytes[i];
			theHash *= 16777619u;
		}
		return theHash;
	}

	uint32_t HashInt(uint32_t theHash, int32_t theValue)
	{
		return HashBytes(theHash, &theValue, sizeof(theValue));
	}

	int FindPacketIndex(Board* theBoard, SeedBank* theBank, SeedType theSeedType, SeedType theImitaterType)
	{
		if (theBank == nullptr)
		{
			return -1;
		}
		for (int i = 0; i < theBank->mNumPackets; i++)
		{
			SeedPacket& aPacket = theBank->mSeedPackets[i];
			if (aPacket.mPacketType == theSeedType && aPacket.mImitaterType == theImitaterType)
			{
				return i;
			}
		}
		return -1;
	}

	bool IsZombieCardSeed(SeedType theSeedType)
	{
		return Challenge::IsZombieSeedType(theSeedType);
	}
}

Versus::Versus(Board* theBoard, VersusSession* theSession, const VersusMatchConfig& theConfig)
{
	mBoard = theBoard;
	mApp = theBoard->mApp;
	mSession = theSession;
	mConfig = theConfig;
	mLocalSide = theConfig.mLocalSide;
	mLastSentTick = 0;
	mNextCommandTick = 1;
	mLastChecksumTick = 0;
	mEnded = false;
	mStarted = false;
	mLocalWinNotified = false;
	mResult = VersusProtocol::RESULT_NONE;
	mBrainProduced = 0;
	mSimRngActive = false;
	mSimRand.SRand(mConfig.mSeed == 0 ? 12345 : mConfig.mSeed);
	for (int i = 0; i < VERSUS_GRAVE_ROWS; i++)
	{
		mGraveCounter[i] = 300 + i * 120;
		mGraveRespawnCounter[i] = 0;
	}
}

Versus::~Versus()
{
}

void Versus::InitLevel()
{
	mBoard->mSunMoney = mConfig.mStartSun;
	mBoard->mZombieBrain = mConfig.mStartBrain;
	mBoard->mSodPosition = 1000;
	mBoard->mNumWaves = 0;
	mBoard->mCurrentWave = 0;
	mBoard->mZombieCountDown = 0x7FFFFFFF;
	mBoard->mZombieCountDownStart = mBoard->mZombieCountDown;
	mBoard->mShowShovel = true;
	mBoard->mEnableGraveStones = false;
	mBoard->mLevelAwardSpawned = false;

	// 背景资源与选择卡牌的僵尸/植物资源预加载
	for (std::string& aResource : mBoard->mLoadedResourceNames)
	{
		TodLoadResources(aResource.c_str());
	}
	ReanimatorEnsureDefinitionLoaded(ReanimationType::REANIM_SUN, true);
	for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
	{
		if (mConfig.mPlantSeeds[i] >= 0)
		{
			Plant::PreloadPlantResources(static_cast<SeedType>(mConfig.mPlantSeeds[i]));
			if (mConfig.mPlantSeeds[i] == SeedType::SEED_IMITATER && mConfig.mPlantImitater[i] >= 0)
			{
				Plant::PreloadPlantResources(static_cast<SeedType>(mConfig.mPlantImitater[i]));
			}
		}
		if (mConfig.mZombieSeeds[i] >= 0)
		{
			Zombie::PreloadZombieResources(Challenge::IZombieSeedTypeToZombieType(static_cast<SeedType>(mConfig.mZombieSeeds[i])));
		}
	}
	for (int i = 0; i < gVersusZombieCardCount; i++)
	{
		for (int j = 0; j < VersusProtocol::CARD_SLOTS; j++)
		{
			if (mConfig.mZombieSeeds[j] == gVersusZombieCards[i].mSeedType)
			{
				Zombie::PreloadZombieResources(static_cast<ZombieType>(gVersusZombieCards[i].mZombieType));
			}
		}
	}

	mBoard->mSeedBank->Move(SEED_BANK_OFFSET_X_END, 0);
	mBoard->mSeedBank->mCutSceneDarken = 128;
	mBoard->mSeedBank->mNumPackets = VersusProtocol::CARD_SLOTS;
	mBoard->mSeedBank->UpdateWidth();
	for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
	{
		SeedPacket& aPacket = mBoard->mSeedBank->mSeedPackets[i];
		aPacket.mIndex = i;
		aPacket.mX = mBoard->GetSeedPacketPositionX(i);
		aPacket.mY = 8;
		SeedType aSeed = static_cast<SeedType>(mConfig.mPlantSeeds[i]);
		SeedType aImitater = static_cast<SeedType>(mConfig.mPlantImitater[i]);
		if (aSeed >= 0)
		{
			aPacket.SetPacketType(aSeed, aImitater);
		}
		else
		{
			aPacket.mPacketType = SeedType::SEED_NONE;
		}
	}

	if (mBoard->mZombieSeedBank != nullptr)
	{
		mBoard->mZombieSeedBank->Move(SEED_BANK_OFFSET_X_END, 0);
		mBoard->mZombieSeedBank->mCutSceneDarken = 128;
		mBoard->mZombieSeedBank->mNumPackets = VersusProtocol::CARD_SLOTS;
		mBoard->mZombieSeedBank->UpdateWidth();
		for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
		{
			SeedPacket& aPacket = mBoard->mZombieSeedBank->mSeedPackets[i];
			aPacket.mIndex = i;
			aPacket.mX = mBoard->GetSeedPacketPositionX(i);
			aPacket.mY = 8;
			if (mConfig.mZombieSeeds[i] >= 0)
			{
				aPacket.SetPacketType(static_cast<SeedType>(mConfig.mZombieSeeds[i]));
			}
			else
			{
				aPacket.mPacketType = SeedType::SEED_NONE;
			}
		}
	}

	SetupGraves();
}

void Versus::SetupBanks()
{
}

void Versus::SetupGraves()
{
	for (int aRow = 0; aRow < VERSUS_GRAVE_ROWS; aRow++)
	{
		if (mBoard->GetGraveStoneAt(VERSUS_GRAVE_COL, aRow) == nullptr)
		{
			mBoard->AddAGraveStone(VERSUS_GRAVE_COL, aRow);
		}
	}
}

void Versus::UpdateGraves()
{
	for (int aRow = 0; aRow < VERSUS_GRAVE_ROWS; aRow++)
	{
		GridItem* aGrave = mBoard->GetGraveStoneAt(VERSUS_GRAVE_COL, aRow);
		if (aGrave == nullptr)
		{
			if (mGraveRespawnCounter[aRow] > 0)
			{
				mGraveRespawnCounter[aRow]--;
				if (mGraveRespawnCounter[aRow] == 0 && mBoard->GetGraveStoneAt(VERSUS_GRAVE_COL, aRow) == nullptr)
				{
					mBoard->AddAGraveStone(VERSUS_GRAVE_COL, aRow);
					mGraveCounter[aRow] = VERSUS_BRAIN_INTERVAL;
				}
			}
			continue;
		}

		if (--mGraveCounter[aRow] <= 0)
		{
			mGraveCounter[aRow] = VERSUS_BRAIN_INTERVAL;
			int aPosX = mBoard->GridToPixelX(VERSUS_GRAVE_COL, aRow) + 20;
			int aPosY = mBoard->GridToPixelY(VERSUS_GRAVE_COL, aRow) - 15;
			mBoard->AddCoin(aPosX, aPosY, CoinType::COIN_BRAIN, CoinMotion::COIN_MOTION_FROM_PLANT);
			mGraveRespawnCounter[aRow] = 0;
			mBrainProduced++;
		}
	}

	// 被墓碑吞噬者清掉后，30 秒后原地重生
	for (int aRow = 0; aRow < VERSUS_GRAVE_ROWS; aRow++)
	{
		if (mBoard->GetGraveStoneAt(VERSUS_GRAVE_COL, aRow) == nullptr && mGraveRespawnCounter[aRow] == 0)
		{
			mGraveRespawnCounter[aRow] = VERSUS_GRAVE_RESPAWN;
		}
	}
}

void Versus::PumpNetwork()
{
	if (mSession == nullptr)
	{
		return;
	}
	mSession->Update();

	if (mSession->IsMatchEnded() && !mEnded)
	{
		mEnded = true;
		mResult = mSession->GetResult();
		return;
	}

	if (!mStarted && mSession->IsGo())
	{
		mStarted = true;
		mLastSentTick = 0;
		mNextCommandTick = 1 + mConfig.mInputDelay;
		// 预填空帧，保证双方前几 tick 都能推进
		for (uint32_t aTick = 1; aTick <= static_cast<uint32_t>(mConfig.mInputDelay); aTick++)
		{
			VersusProtocol::InputFrame aFrame;
			aFrame.mTick = aTick;
			auto aIt = mPendingCommands.find(aTick);
			if (aIt != mPendingCommands.end())
			{
				aFrame.mCommands = aIt->second;
				mPendingCommands.erase(aIt);
			}
			mLocalFrames[aTick] = aFrame;
			mSession->SendInputFrame(aFrame);
			mLastSentTick = aTick;
		}
	}

	if (mStarted && !mEnded)
	{
		SendFramesUpTo(mBoard->mMainCounter + static_cast<uint32_t>(mConfig.mInputDelay));
	}
}

void Versus::SendFramesUpTo(uint32_t theTick)
{
	if (theTick <= mLastSentTick)
	{
		return;
	}
	for (uint32_t aTick = mLastSentTick + 1; aTick <= theTick; aTick++)
	{
		VersusProtocol::InputFrame aFrame;
		aFrame.mTick = aTick;
		auto aIt = mPendingCommands.find(aTick);
		if (aIt != mPendingCommands.end())
		{
			aFrame.mCommands = aIt->second;
			mPendingCommands.erase(aIt);
		}
		mLocalFrames[aTick] = aFrame;
		mSession->SendInputFrame(aFrame);
	}
	mLastSentTick = theTick;
}

bool Versus::CanStepSimulation()
{
	if (!mStarted || mEnded || mSession == nullptr)
	{
		return false;
	}
	uint32_t aTick = mBoard->mMainCounter + 1;
	if (mLocalFrames.find(aTick) == mLocalFrames.end())
	{
		return false;
	}
	return mSession->HasRemoteFrame(aTick);
}

void Versus::ApplyCommandsForTick(uint32_t theTick)
{
	auto aLocalIt = mLocalFrames.find(theTick);
	const VersusProtocol::InputFrame* aLocal = aLocalIt == mLocalFrames.end() ? nullptr : &aLocalIt->second;
	const VersusProtocol::InputFrame* aRemote = mSession->GetRemoteFrame(theTick);
	if (aLocal == nullptr || aRemote == nullptr)
	{
		return;
	}

	const VersusProtocol::InputFrame* aHostFrame = mSession->IsHost() ? aLocal : aRemote;
	const VersusProtocol::InputFrame* aGuestFrame = mSession->IsHost() ? aRemote : aLocal;
	for (const VersusProtocol::InputFrame* aFrame : { aHostFrame, aGuestFrame })
	{
		for (const VersusProtocol::Command& aCommand : aFrame->mCommands)
		{
			switch (aCommand.mType)
			{
			case VersusProtocol::CMD_PLANT:
				ApplyPlantCommand(aCommand);
				break;
			case VersusProtocol::CMD_ZOMBIE:
				ApplyZombieCommand(aCommand);
				break;
			case VersusProtocol::CMD_SHOVEL:
				ApplyShovelCommand(aCommand);
				break;
			default:
				break;
			}
		}
	}

	mLocalFrames.erase(theTick);
}

bool Versus::ApplyPlantCommand(const VersusProtocol::Command& theCommand)
{
	SeedType aSeed = static_cast<SeedType>(theCommand.mSeedType);
	if (aSeed < 0 || IsZombieCardSeed(aSeed))
	{
		return false;
	}
	int aGridX = theCommand.mGridX;
	int aGridY = theCommand.mGridY;
	SeedType aCheckSeed = (aSeed == SeedType::SEED_IMITATER && theCommand.mImitaterType >= 0) ? static_cast<SeedType>(theCommand.mImitaterType) : aSeed;
	PlantingReason aReason = mBoard->CanPlantAt(aGridX, aGridY, aCheckSeed);
	if (aReason != PlantingReason::PLANTING_OK)
	{
		return false;
	}
	SeedType aImitater = static_cast<SeedType>(theCommand.mImitaterType);
	int aPacketIndex = FindPacketIndex(mBoard, mBoard->mSeedBank, aSeed, aImitater);
	if (aPacketIndex < 0 || !mBoard->mSeedBank->mSeedPackets[aPacketIndex].mActive)
	{
		return false;
	}
	int aCost = Plant::GetCost(aSeed, aImitater);
	if (!mBoard->CanTakeSunMoney(aCost))
	{
		return false;
	}

	// 复用原版种植逻辑（含扣阳光、升级、南瓜头、花盆等），不要在这里重复扣费
	CursorObject* aCursor = mBoard->mCursorObject;
	SeedType aOldType = aCursor->mType;
	SeedType aOldImitater = aCursor->mImitaterType;
	CursorType aOldCursorType = aCursor->mCursorType;
	int aOldBankIndex = aCursor->mSeedBankIndex;
	aCursor->mType = aSeed;
	aCursor->mImitaterType = aImitater;
	aCursor->mCursorType = CursorType::CURSOR_TYPE_PLANT_FROM_BANK;
	aCursor->mSeedBankIndex = aPacketIndex;
	int aPixelX = mBoard->GridToPixelX(aGridX, aGridY);
	int aPixelY = mBoard->GridToPixelY(aGridX, aGridY);
	mBoard->MouseDownWithPlant(aPixelX, aPixelY, 1);
	aCursor->mType = aOldType;
	aCursor->mImitaterType = aOldImitater;
	aCursor->mCursorType = aOldCursorType;
	aCursor->mSeedBankIndex = aOldBankIndex;
	return true;
}

bool Versus::ApplyZombieCommand(const VersusProtocol::Command& theCommand)
{
	SeedType aSeed = static_cast<SeedType>(theCommand.mSeedType);
	if (!IsZombieCardSeed(aSeed))
	{
		return false;
	}
	int aGridX = theCommand.mGridX;
	int aGridY = theCommand.mGridY;
	SeedType aCheckSeed = (aSeed == SeedType::SEED_IMITATER && theCommand.mImitaterType >= 0) ? static_cast<SeedType>(theCommand.mImitaterType) : aSeed;
		if (mBoard->CanPlantAt(aGridX, aGridY, aCheckSeed) != PlantingReason::PLANTING_OK)
	{
		return false;
	}
	int aPacketIndex = FindPacketIndex(mBoard, mBoard->mZombieSeedBank, aSeed, SeedType::SEED_NONE);
	if (aPacketIndex < 0)
	{
		return false;
	}
	int aCost = Plant::GetCost(aSeed, SeedType::SEED_NONE);
	if (!mBoard->TakeBrainLight(aCost))
	{
		return false;
	}
	ZombieType aZombieType = Challenge::IZombieSeedTypeToZombieType(aSeed);
	mBoard->mChallenge->IZombiePlaceZombie(aZombieType, aGridX, aGridY);
	mBoard->mZombieSeedBank->mSeedPackets[aPacketIndex].mTimesUsed++;
	return true;
}

bool Versus::ApplyShovelCommand(const VersusProtocol::Command& theCommand)
{
	Plant* aPlant = mBoard->GetTopPlantAt(theCommand.mGridX, theCommand.mGridY, PlantPriority::TOPPLANT_ANY);
	if (aPlant == nullptr)
	{
		return false;
	}
	aPlant->Die();
	mBoard->mPlantsShoveled++;
	return true;
}

void Versus::Update()
{
	if (mEnded)
	{
		return;
	}

	if (mSession != nullptr && mSession->IsMatchEnded())
	{
		mEnded = true;
		mResult = mSession->GetResult();
		return;
	}

	UpdateGraves();
	UpdateMatchTimer();

	if (mBoard->mMainCounter % 100 == 0 && mBoard->mMainCounter != mLastChecksumTick)
	{
		mLastChecksumTick = mBoard->mMainCounter;
		uint32_t aHash = ComputeChecksum();
		mSession->SendChecksum(mBoard->mMainCounter, aHash);
		if (mSession->HasRemoteChecksum(mBoard->mMainCounter))
		{
			uint32_t aRemote = mSession->GetRemoteChecksum(mBoard->mMainCounter);
			if (aRemote != aHash)
			{
				EndMatch(VersusProtocol::RESULT_ABORT, true);
			}
		}
	}
}

void Versus::UpdateMatchTimer()
{
	if (mBoard->mMainCounter >= static_cast<uint32_t>(mConfig.mDurationTicks))
	{
		EndMatch(VersusProtocol::RESULT_PLANT_WIN, true);
	}
}

void Versus::OnZombieReachedHouse(Zombie* theZombie)
{
	(void)theZombie;
	EndMatch(VersusProtocol::RESULT_ZOMBIE_WIN, true);
}

void Versus::EndMatch(uint8_t theResult, bool theNotifyPeer)
{
	if (mEnded)
	{
		return;
	}
	mEnded = true;
	mResult = theResult;
	if (theNotifyPeer && mSession != nullptr)
	{
		mSession->SendResult(theResult);
	}
}

int Versus::GetRemainingTicks() const
{
	int32_t aRemaining = mConfig.mDurationTicks - static_cast<int32_t>(mBoard->mMainCounter);
	return aRemaining < 0 ? 0 : aRemaining;
}

uint32_t Versus::ComputeChecksum() const
{
	uint32_t aHash = 2166136261u;
	aHash = HashInt(aHash, static_cast<int32_t>(mBoard->mMainCounter));
	aHash = HashInt(aHash, mBoard->mSunMoney);
	aHash = HashInt(aHash, mBoard->mZombieBrain);

	for (int i = 0; i < mBoard->mSeedBank->mNumPackets; i++)
	{
		const SeedPacket& aPacket = mBoard->mSeedBank->mSeedPackets[i];
		aHash = HashInt(aHash, aPacket.mPacketType);
		aHash = HashInt(aHash, aPacket.mRefreshCounter);
		aHash = HashInt(aHash, aPacket.mActive ? 1 : 0);
	}
	if (mBoard->mZombieSeedBank != nullptr)
	{
		for (int i = 0; i < mBoard->mZombieSeedBank->mNumPackets; i++)
		{
			aHash = HashInt(aHash, mBoard->mZombieSeedBank->mSeedPackets[i].mPacketType);
		}
	}

	int aPlantCount = 0;
	Plant* aPlant = nullptr;
	while (mBoard->IteratePlants(aPlant))
	{
		aPlantCount++;
		aHash = HashInt(aHash, aPlant->mSeedType);
		aHash = HashInt(aHash, aPlant->mPlantCol);
		aHash = HashInt(aHash, aPlant->mRow);
		aHash = HashInt(aHash, aPlant->mPlantHealth);
	}
	aHash = HashInt(aHash, aPlantCount);

	int aZombieCount = 0;
	Zombie* aZombie = nullptr;
	while (mBoard->IterateZombies(aZombie))
	{
		aZombieCount++;
		aHash = HashInt(aHash, aZombie->mZombieType);
		aHash = HashInt(aHash, aZombie->mRow);
		aHash = HashInt(aHash, aZombie->mBodyHealth + aZombie->mHelmHealth + aZombie->mShieldHealth);
		aHash = HashBytes(aHash, &aZombie->mPosX, sizeof(aZombie->mPosX));
	}
	aHash = HashInt(aHash, aZombieCount);

	GridItem* aGridItem = nullptr;
	int aGraveCount = 0;
	while (mBoard->IterateGridItems(aGridItem))
	{
		if (aGridItem->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE)
		{
			aGraveCount++;
			aHash = HashInt(aHash, aGridItem->mGridX);
			aHash = HashInt(aHash, aGridItem->mGridY);
		}
	}
	aHash = HashInt(aHash, aGraveCount);
	return aHash;
}

void Versus::BeginSimRng()
{
	if (mSimRngActive || mEnded)
	{
		return;
	}
	mSavedUiRand = Sexy::GetRandState();
	Sexy::SetRandState(mSimRand);
	mSimRngActive = true;
}

void Versus::EndSimRng()
{
	if (!mSimRngActive)
	{
		return;
	}
	mSimRand = Sexy::GetRandState();
	Sexy::SetRandState(mSavedUiRand);
	mSimRngActive = false;
}

void Versus::QueueLocalCommand(const VersusProtocol::Command& theCommand)
{
	uint32_t aTarget = mBoard->mMainCounter + static_cast<uint32_t>(mConfig.mInputDelay);
	// 已经发出去的 tick 不能再塞命令，顺延到下一个尚未发送的 tick
	if (aTarget <= mLastSentTick)
	{
		aTarget = mLastSentTick + 1;
	}
	if (aTarget < mNextCommandTick)
	{
		aTarget = mNextCommandTick;
	}
	auto& aCommands = mPendingCommands[aTarget];
	if (aCommands.size() >= static_cast<size_t>(VersusProtocol::MAX_COMMANDS_PER_FRAME))
	{
		aTarget++;
		mPendingCommands[aTarget].push_back(theCommand);
	}
	else
	{
		aCommands.push_back(theCommand);
	}
	mNextCommandTick = aTarget + 1;
}

bool Versus::MouseDown(int theX, int theY, int theClickCount)
{
	if (mEnded)
	{
		if (theClickCount > 0 && theX >= 300 && theX < 500 && theY >= 360 && theY < 400)
		{
			mApp->RequestBackToMain();
		}
		return true;
	}

	if (mBoard->mMenuButton->IsMouseOver())
	{
		if (mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "退出对战", "确定要退出本局对战吗？\n对手将获得胜利。", "[DIALOG_BUTTON_YES]", "[DIALOG_BUTTON_NO]", Dialog::BUTTONS_YES_NO) == Dialog::ID_YES)
		{
			EndMatch(IsLocalPlant() ? VersusProtocol::RESULT_ZOMBIE_WIN : VersusProtocol::RESULT_PLANT_WIN, true);
			mApp->RequestBackToMain();
		}
		return true;
	}

	if (IsLocalPlant())
	{
		HitResult aHit;
		if (mBoard->mSeedBank->MouseHitTest(theX, theY, &aHit) && aHit.mObjectType == GameObjectType::OBJECT_TYPE_SEEDPACKET)
		{
			SeedPacket* aPacket = (SeedPacket*)aHit.mObject;
			if (aPacket->CanPickUp())
			{
				mBoard->mCursorObject->mType = aPacket->mPacketType;
				mBoard->mCursorObject->mImitaterType = aPacket->mImitaterType;
				mBoard->mCursorObject->mCursorType = CursorType::CURSOR_TYPE_PLANT_FROM_BANK;
				mBoard->mCursorObject->mSeedBankIndex = aPacket->mIndex;
				mApp->PlaySample(SOUND_SEEDLIFT);
			}
			else
			{
				mApp->PlaySample(SOUND_BUZZER);
			}
			return true;
		}
		Rect aShovelRect = mBoard->GetShovelButtonRect();
		if (mBoard->mShowShovel && aShovelRect.Contains(theX, theY))
		{
			mBoard->mCursorObject->mCursorType = CursorType::CURSOR_TYPE_SHOVEL;
			mApp->PlayFoley(FoleyType::FOLEY_USE_SHOVEL);
			return true;
		}
	}
	else
	{
		HitResult aHit;
		if (mBoard->mZombieSeedBank != nullptr && mBoard->mZombieSeedBank->MouseHitTest(theX, theY, &aHit) && aHit.mObjectType == GameObjectType::OBJECT_TYPE_SEEDPACKET)
		{
			SeedPacket* aPacket = (SeedPacket*)aHit.mObject;
			int aCost = Plant::GetCost(aPacket->mPacketType, SeedType::SEED_NONE);
			if (mBoard->CanTakeBrainLight(aCost))
			{
				mBoard->mCursorObject->mType = aPacket->mPacketType;
				mBoard->mCursorObject->mImitaterType = SeedType::SEED_NONE;
				mBoard->mCursorObject->mCursorType = CursorType::CURSOR_TYPE_PLANT_FROM_BANK;
				mBoard->mCursorObject->mSeedBankIndex = aPacket->mIndex;
				mApp->PlaySample(SOUND_SEEDLIFT);
			}
			else
			{
				mApp->PlaySample(SOUND_BUZZER);
			}
			return true;
		}
	}

	SeedType aCursorType = mBoard->GetSeedTypeInCursor();
	if (aCursorType != SeedType::SEED_NONE && theClickCount > 0)
	{
		int aGridX = mBoard->PlantingPixelToGridX(theX, theY, aCursorType);
		int aGridY = mBoard->PlantingPixelToGridY(theX, theY, aCursorType);
		if (aGridX < 0 || aGridX >= MAX_GRID_SIZE_X || aGridY < 0 || aGridY >= MAX_GRID_SIZE_Y)
		{
			mBoard->ClearCursor();
			return true;
		}

		if (IsLocalPlant())
		{
			if (mBoard->CanPlantAt(aGridX, aGridY, aCursorType) == PlantingReason::PLANTING_OK)
			{
				VersusProtocol::Command aCommand;
				aCommand.mType = VersusProtocol::CMD_PLANT;
				// 光标里的 type 对模仿者会变成目标植物；命令必须携带 IMITATER 本体和目标。
				aCommand.mSeedType = static_cast<int8_t>(mBoard->mCursorObject->mType);
				aCommand.mImitaterType = static_cast<int8_t>(mBoard->mCursorObject->mImitaterType);
				aCommand.mGridX = static_cast<uint8_t>(aGridX);
				aCommand.mGridY = static_cast<uint8_t>(aGridY);
				QueueLocalCommand(aCommand);
			}
		}
		else
		{
			if (mBoard->CanPlantAt(aGridX, aGridY, aCursorType) == PlantingReason::PLANTING_OK)
			{
				VersusProtocol::Command aCommand;
				aCommand.mType = VersusProtocol::CMD_ZOMBIE;
				aCommand.mSeedType = static_cast<int8_t>(aCursorType);
				aCommand.mImitaterType = -1;
				aCommand.mGridX = static_cast<uint8_t>(aGridX);
				aCommand.mGridY = static_cast<uint8_t>(aGridY);
				QueueLocalCommand(aCommand);
			}
		}
		mBoard->ClearCursor();
		return true;
	}

	if (mBoard->mCursorObject->mCursorType == CursorType::CURSOR_TYPE_SHOVEL && IsLocalPlant() && theClickCount > 0)
	{
		int aGridX = mBoard->PixelToGridX(theX, theY);
		int aGridY = mBoard->PixelToGridY(theX, theY);
		if (aGridX >= 0 && aGridX < MAX_GRID_SIZE_X && aGridY >= 0 && aGridY < MAX_GRID_SIZE_Y)
		{
			VersusProtocol::Command aCommand;
			aCommand.mType = VersusProtocol::CMD_SHOVEL;
			aCommand.mGridX = static_cast<uint8_t>(aGridX);
			aCommand.mGridY = static_cast<uint8_t>(aGridY);
			QueueLocalCommand(aCommand);
			mBoard->ClearCursor();
			mApp->PlayFoley(FoleyType::FOLEY_USE_SHOVEL);
		}
		return true;
	}

	mBoard->ClearCursor();
	return true;
}

void Versus::MouseMove(int theX, int theY)
{
	(void)theX;
	(void)theY;
}

void Versus::MouseUp(int theX, int theY, int theClickCount)
{
	(void)theX;
	(void)theY;
	(void)theClickCount;
}

void Versus::UpdateCursor()
{
}

void Versus::DrawHUD(Sexy::Graphics* g)
{
	bool aZombie = IsLocalZombie();
	if (!aZombie)
	{
		if (mBoard->mSeedBank->BeginDraw(g))
		{
			mBoard->mSeedBank->Draw(g);
			mBoard->mSeedBank->EndDraw(g);
		}
	}
	else
	{
		g->DrawImage(Sexy::IMAGE_SEEDBANK, 0, 0);
		for (int i = 0; i < (mBoard->mZombieSeedBank ? mBoard->mZombieSeedBank->mNumPackets : 0); i++)
		{
			SeedPacket& aPacket = mBoard->mZombieSeedBank->mSeedPackets[i];
			if (aPacket.mPacketType == SeedType::SEED_NONE)
			{
				continue;
			}
			int aCost = Plant::GetCost(aPacket.mPacketType, SeedType::SEED_NONE);
			int aGrayness = mBoard->CanTakeBrainLight(aCost) ? 255 : 128;
			g->SetColor(Color::White);
			TodDrawImageCelScaledF(g, Sexy::IMAGE_SEEDS, static_cast<float>(aPacket.mX), 8.0f, 2, 0, g->mScaleX, g->mScaleY);
			if (aGrayness != 255)
			{
				g->SetColor(Color(0, 0, 0, 90));
				g->FillRect(aPacket.mX, 8, 50, 70);
			}
			g->SetColor(Color::White);
			SeedPacketDrawSeed(g, static_cast<float>(aPacket.mX), 8.0f, aPacket.mPacketType, SeedType::SEED_NONE, 4.0f, 8.0f, 0.22f);
			g->SetColor(Color::White);
			std::string aCostText = std::to_string(aCost);
			TodDrawString(g, aCostText, aPacket.mX + 25, 73, Sexy::FONT_CONTINUUMBOLD14, Color(255, 170, 230), DS_ALIGN_CENTER);
		}
		// 僵尸卡槽图自带阳光图标，先用木板色盖住，再画脑光图标
		g->SetColor(Color(96, 50, 24));
		g->FillRect(10, 10, 78, 46);
		if (Sexy::IMAGE_BRAIN)
		{
			TodDrawImageScaledF(g, Sexy::IMAGE_BRAIN, 24.0f, 10.0f, 0.8f, 0.8f);
		}
		std::string aBrainText = std::to_string(std::max(mBoard->mZombieBrain, 0));
		TodDrawString(g, aBrainText, 37, 78, Sexy::FONT_CONTINUUMBOLD14, Color(255, 120, 210), DS_ALIGN_CENTER);
	}

	int aRemaining = GetRemainingTicks();
	int aSeconds = (aRemaining + 99) / 100;
	std::string aTime = StrFormat("剩余 %d:%02d", aSeconds / 60, aSeconds % 60);
	TodDrawString(g, aTime, 400, 112, Sexy::FONT_DWARVENTODCRAFT18YELLOW, Color(255, 240, 150), DS_ALIGN_CENTER);
	TodDrawString(g, IsLocalPlant() ? "你是植物方" : "你是僵尸方", 400, 134, Sexy::FONT_BRIANNETOD12, Color::White, DS_ALIGN_CENTER);

	if (!mStarted)
	{
		g->SetColor(Color(0, 0, 0, 150));
		g->FillRect(0, 0, BOARD_WIDTH, BOARD_HEIGHT);
		TodDrawString(g, "等待对手加载...", 400, 290, Sexy::FONT_DWARVENTODCRAFT18YELLOW, Color::White, DS_ALIGN_CENTER);
	}
}

void Versus::DrawOverlay(Sexy::Graphics* g)
{
	if (!mEnded)
	{
		return;
	}

	g->SetColor(Color(0, 0, 0, 170));
	g->FillRect(0, 0, BOARD_WIDTH, BOARD_HEIGHT);
	std::string aText;
	if (mResult == VersusProtocol::RESULT_PLANT_WIN)
	{
		aText = "植物方胜利！";
	}
	else if (mResult == VersusProtocol::RESULT_ZOMBIE_WIN)
	{
		aText = "僵尸方胜利！";
	}
	else if (mResult == VersusProtocol::RESULT_ABORT)
	{
		aText = "同步失败，比赛结束";
	}
	else
	{
		aText = "对战结束";
	}
	TodDrawString(g, aText, 400, 280, Sexy::FONT_DWARVENTODCRAFT18YELLOW, Color(255, 240, 150), DS_ALIGN_CENTER);
	g->SetColor(Color(60, 60, 60));
	g->FillRect(300, 360, 200, 40);
	g->SetColor(Color(200, 200, 200));
	g->DrawRect(300, 360, 200, 40);
	TodDrawString(g, "返回主菜单", 400, 385, Sexy::FONT_BRIANNETOD12, Color::White, DS_ALIGN_CENTER);
}
