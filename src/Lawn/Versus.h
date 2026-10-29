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

#ifndef __VERSUS_H__
#define __VERSUS_H__

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "../ConstEnums.h"
#include "../SexyAppFramework/misc/MTRand.h"
#include "System/VersusNetwork.h"
#include "System/VersusProtocol.h"

class LawnApp;
class Board;
class Zombie;
namespace Sexy { class Graphics; }

constexpr int VERSUS_PORT = 27315;
constexpr int VERSUS_TICK_RATE = 100;
constexpr int VERSUS_MATCH_TICKS = 18000;
constexpr int VERSUS_START_SUN = 150;
constexpr int VERSUS_START_BRAIN = 150;
constexpr int VERSUS_BRAIN_VALUE = 20;
constexpr int VERSUS_BRAIN_INTERVAL = 1000;
constexpr int VERSUS_GRAVE_ROWS = 6;
constexpr int VERSUS_GRAVE_COL = 8;
constexpr int VERSUS_GRAVE_RESPAWN = 3000;
constexpr int VERSUS_ZOMBIE_MIN_COL = 4;
constexpr int VERSUS_DEFAULT_INPUT_DELAY = 4;
constexpr int VERSUS_MAX_INPUT_DELAY = 10;

struct VersusMatchConfig
{
	uint32_t mSeed = 0;
	uint8_t mLocalSide = VersusProtocol::SIDE_NONE;
	uint8_t mHostSide = VersusProtocol::SIDE_NONE;
	int32_t mInputDelay = VERSUS_DEFAULT_INPUT_DELAY;
	int32_t mDurationTicks = VERSUS_MATCH_TICKS;
	int32_t mStartSun = VERSUS_START_SUN;
	int32_t mStartBrain = VERSUS_START_BRAIN;
	int8_t mPlantSeeds[VersusProtocol::CARD_SLOTS] = {};
	int8_t mPlantImitater[VersusProtocol::CARD_SLOTS] = {};
	int8_t mZombieSeeds[VersusProtocol::CARD_SLOTS] = {};
	uint32_t mRulesFlags = 0;
};

struct VersusZombieCard
{
	int8_t mSeedType;
	int8_t mZombieType;
	int32_t mCost;
	const char* mName;
	bool mWaterOnly;
};

extern const VersusZombieCard gVersusZombieCards[];
extern const int gVersusZombieCardCount;

class VersusSession
{
public:
	enum class Phase
	{
		Idle,
		Hosting,
		Connecting,
		Lobby,
		Loading,
		Playing,
		Result,
		Error
	};

	VersusSession();
	~VersusSession();

	bool StartHost();
	bool StartJoin(const std::string& theHost);
	void Close();
	void Update();

	bool SendSide(uint8_t theSide);
	void ReleaseLocalSide();
	void ResetSides();
	void SetLocalLoadout(const int* thePlantSeeds, const int* thePlantImitater, const int* theZombieSeeds);
	const int* GetLocalPlantSeeds() const { return mLocalPlantSeeds; }
	const int* GetLocalPlantImitater() const { return mLocalPlantImitater; }
	const int* GetLocalZombieSeeds() const { return mLocalZombieSeeds; }
	bool SendPick(bool theReady);
	bool StartMatch();
	bool SendLoaded();
	void SendInputFrame(const VersusProtocol::InputFrame& theFrame);
	void SendChecksum(uint32_t theTick, uint32_t theHash);
	void SendResult(uint8_t theResult);

	bool TakeStartRequest();
	bool TakeGoRequest();
	bool IsMatchEnded() const { return mResultReceived; }
	uint8_t GetResult() const { return mResult; }
	bool IsGo() const { return mGo; }
	bool IsHost() const { return mIsHost; }
	Phase GetPhase() const { return mPhase; }
	const std::string& GetStatus() const { return mStatus; }
	const std::string& GetRemoteName() const { return mRemoteName; }
	uint8_t GetLocalSide() const { return mLocalSide; }
	uint8_t GetRemoteSide() const { return mRemoteSide; }
	bool IsLocalReady() const { return mLocalReady; }
	bool IsRemoteReady() const { return mRemoteReady; }
	bool AreSidesReady() const;
	bool CanStartMatch() const;
	VersusMatchConfig& GetConfig() { return mConfig; }
	const VersusMatchConfig& GetConfig() const { return mConfig; }
	uint8_t GetLocalResult() const { return mLocalResult; }
	bool HasRemoteFrame(uint32_t theTick) const;
	const VersusProtocol::InputFrame* GetRemoteFrame(uint32_t theTick) const;
	bool HasRemoteChecksum(uint32_t theTick) const;
	uint32_t GetRemoteChecksum(uint32_t theTick) const;
	void SetLocalPlayerName(const std::string& theName) { mLocalName = theName; }
	int GetPingMs() const { return mPingMs; }
	VersusNetwork& GetNetwork() { return mNetwork; }

private:
	VersusNetwork mNetwork;
	Phase mPhase;
	bool mIsHost;
	std::string mStatus;
	std::string mError;
	std::string mLocalName;
	std::string mRemoteName;
	uint8_t mLocalSide;
	uint8_t mRemoteSide;
	bool mLocalReady;
	bool mRemoteReady;
	bool mLocalLoaded;
	bool mRemoteLoaded;
	bool mGo;
	bool mStartRequested;
	bool mGoRequested;
	bool mResultReceived;
	uint8_t mResult;
	uint8_t mLocalResult;
	bool mGotHello;
	uint32_t mSeed;
	int32_t mInputDelay;
	int32_t mPingMs;
	uint64_t mPingSentAt;
	uint64_t mLastPingAt;
	int mLocalPlantSeeds[VersusProtocol::CARD_SLOTS];
	int mLocalPlantImitater[VersusProtocol::CARD_SLOTS];
	int mLocalZombieSeeds[VersusProtocol::CARD_SLOTS];
	int mRemotePlantSeeds[VersusProtocol::CARD_SLOTS];
	int mRemotePlantImitater[VersusProtocol::CARD_SLOTS];
	int mRemoteZombieSeeds[VersusProtocol::CARD_SLOTS];
	VersusMatchConfig mConfig;
	std::map<uint32_t, VersusProtocol::InputFrame> mRemoteFrames;
	std::map<uint32_t, uint32_t> mRemoteChecksums;

	void SetPhase(Phase thePhase, const std::string& theStatus);
	void HandleMessage(uint8_t theType, const uint8_t* thePayload, size_t theSize);
	void SendHello();
	void SendSimple(uint8_t theType);
	void SendStart();
	void SendPickInternal(bool theReady);
	void UpdatePing();
	void StoreLocalLoadout();
};

class Versus
{
public:
	Versus(Board* theBoard, VersusSession* theSession, const VersusMatchConfig& theConfig);
	~Versus();

	void InitLevel();
	void Update();
	void PumpNetwork();
	bool CanStepSimulation();
	void ApplyCommandsForTick(uint32_t theTick);
	void OnZombieReachedHouse(Zombie* theZombie);

	bool MouseDown(int theX, int theY, int theClickCount);
	void MouseMove(int theX, int theY);
	void MouseUp(int theX, int theY, int theClickCount);
	void UpdateCursor();

	void DrawHUD(Sexy::Graphics* g);
	void DrawOverlay(Sexy::Graphics* g);
	void BeginSimRng();
	void EndSimRng();

	bool IsEnded() const { return mEnded; }
	uint32_t ComputeChecksum() const;
	uint8_t GetLocalSide() const { return mLocalSide; }
	uint8_t GetResult() const { return mResult; }
	int GetRemainingTicks() const;
	void EndMatch(uint8_t theResult, bool theNotifyPeer);

private:
	Board* mBoard;
	LawnApp* mApp;
	VersusSession* mSession;
	VersusMatchConfig mConfig;
	uint8_t mLocalSide;
	uint32_t mLastSentTick;
	uint32_t mNextCommandTick;
	uint32_t mLastChecksumTick;
	bool mEnded;
	bool mStarted;
	bool mLocalWinNotified;
	uint8_t mResult;
	int mGraveCounter[VERSUS_GRAVE_ROWS];
	int mGraveRespawnCounter[VERSUS_GRAVE_ROWS];
	int mBrainProduced;
	Sexy::MTRand mSimRand;
	Sexy::MTRand mSavedUiRand;
	bool mSimRngActive;
	std::map<uint32_t, std::vector<VersusProtocol::Command>> mPendingCommands;
	std::map<uint32_t, VersusProtocol::InputFrame> mLocalFrames;

	void SetupBanks();
	void SetupGraves();
	void UpdateGraves();
	void UpdateMatchTimer();
	void QueueLocalCommand(const VersusProtocol::Command& theCommand);
	void SendFramesUpTo(uint32_t theTick);
	bool ApplyPlantCommand(const VersusProtocol::Command& theCommand);
	bool ApplyZombieCommand(const VersusProtocol::Command& theCommand);
	bool ApplyShovelCommand(const VersusProtocol::Command& theCommand);
	void UpdateZombieCursor();
	bool IsLocalZombie() const { return mLocalSide == VersusProtocol::SIDE_ZOMBIE; }
	bool IsLocalPlant() const { return mLocalSide == VersusProtocol::SIDE_PLANT; }
};

#endif
