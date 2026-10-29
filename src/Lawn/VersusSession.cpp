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

#include <chrono>
#include <cstring>
#include <random>

namespace
{
	uint64_t NowMs()
	{
		using namespace std::chrono;
		return static_cast<uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
	}

	bool IsValidSide(uint8_t theSide)
	{
		return theSide == VersusProtocol::SIDE_PLANT || theSide == VersusProtocol::SIDE_ZOMBIE;
	}

	uint8_t OppositeSide(uint8_t theSide)
	{
		return theSide == VersusProtocol::SIDE_PLANT ? VersusProtocol::SIDE_ZOMBIE : VersusProtocol::SIDE_PLANT;
	}
}

VersusSession::VersusSession()
{
	mPhase = Phase::Idle;
	mIsHost = false;
	mLocalSide = VersusProtocol::SIDE_NONE;
	mRemoteSide = VersusProtocol::SIDE_NONE;
	mLocalReady = false;
	mRemoteReady = false;
	mLocalLoaded = false;
	mRemoteLoaded = false;
	mGo = false;
	mStartRequested = false;
	mGoRequested = false;
	mResultReceived = false;
	mResult = VersusProtocol::RESULT_NONE;
	mLocalResult = VersusProtocol::RESULT_NONE;
	mGotHello = false;
	mSeed = 0;
	mInputDelay = VERSUS_DEFAULT_INPUT_DELAY;
	mPingMs = 0;
	mPingSentAt = 0;
	mLastPingAt = 0;
	for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
	{
		mLocalPlantSeeds[i] = -1;
		mLocalPlantImitater[i] = -1;
		mLocalZombieSeeds[i] = -1;
		mRemotePlantSeeds[i] = -1;
		mRemotePlantImitater[i] = -1;
		mRemoteZombieSeeds[i] = -1;
	}
	memset(&mConfig, 0, sizeof(mConfig));
	mConfig.mLocalSide = VersusProtocol::SIDE_NONE;
	mConfig.mHostSide = VersusProtocol::SIDE_NONE;
}

VersusSession::~VersusSession()
{
	Close();
}

void VersusSession::SetPhase(Phase thePhase, const std::string& theStatus)
{
	mPhase = thePhase;
	mStatus = theStatus;
}

bool VersusSession::StartHost()
{
	Close();
	mIsHost = true;
	std::string aError;
	if (!mNetwork.Host(VERSUS_PORT, aError))
	{
		SetPhase(Phase::Error, aError);
		return false;
	}
	SetPhase(Phase::Hosting, "房间已创建，等待对手加入");
	return true;
}

bool VersusSession::StartJoin(const std::string& theHost)
{
	Close();
	mIsHost = false;
	std::string aError;
	if (!mNetwork.Join(theHost, VERSUS_PORT, aError))
	{
		SetPhase(Phase::Error, aError);
		return false;
	}
	SetPhase(Phase::Connecting, "正在连接主机...");
	return true;
}

void VersusSession::Close()
{
	if (mNetwork.IsConnected())
	{
		mNetwork.SendPacket(VersusProtocol::MSG_BYE, std::vector<uint8_t>());
	}
	mNetwork.Close();
	mPhase = Phase::Idle;
	mIsHost = false;
	mLocalSide = VersusProtocol::SIDE_NONE;
	mRemoteSide = VersusProtocol::SIDE_NONE;
	mLocalReady = false;
	mRemoteReady = false;
	mLocalLoaded = false;
	mRemoteLoaded = false;
	mGo = false;
	mStartRequested = false;
	mGoRequested = false;
	mResultReceived = false;
	mResult = VersusProtocol::RESULT_NONE;
	mLocalResult = VersusProtocol::RESULT_NONE;
	mGotHello = false;
	mRemoteFrames.clear();
	mRemoteChecksums.clear();
	mStatus.clear();
	mError.clear();
	mRemoteName.clear();
}

void VersusSession::Update()
{
	mNetwork.Update();

	if (mNetwork.GetState() == VersusNetwork::State::Error)
	{
		if (mPhase == Phase::Playing || mPhase == Phase::Loading)
		{
			mResultReceived = true;
			mResult = mLocalSide;
			mLocalResult = mResult;
			SetPhase(Phase::Result, mNetwork.GetError());
		}
		else
		{
			SetPhase(Phase::Error, mNetwork.GetError());
		}
	}

	if (mNetwork.IsConnected())
	{
		if (mPhase == Phase::Connecting)
		{
			SendHello();
			SetPhase(Phase::Lobby, "已连接，等待主机确认");
		}
		else if (mPhase == Phase::Hosting)
		{
			SetPhase(Phase::Lobby, "对手已连接，等待握手");
		}
	}

	UpdatePing();

	std::vector<uint8_t> aMessage;
	while (mNetwork.PollMessage(aMessage))
	{
		uint8_t aType = 0;
		const uint8_t* aPayload = nullptr;
		size_t aPayloadSize = 0;
		if (!VersusProtocol::ParseMessage(aMessage, aType, aPayload, aPayloadSize))
		{
			continue;
		}
		HandleMessage(aType, aPayload, aPayloadSize);
	}
}

void VersusSession::UpdatePing()
{
	if (!mNetwork.IsConnected() || mPhase == Phase::Result || mPhase == Phase::Error)
	{
		return;
	}
	uint64_t aNow = NowMs();
	if (mPingSentAt != 0 && aNow - mPingSentAt < 4000)
	{
		return;
	}
	if (aNow - mLastPingAt < 1000)
	{
		return;
	}
	mLastPingAt = aNow;
	mPingSentAt = aNow;
	VersusProtocol::Writer aWriter;
	aWriter.WriteU32(0);
	mNetwork.SendPacket(VersusProtocol::MSG_PING, aWriter.mData);
}

void VersusSession::SendHello()
{
	VersusProtocol::Writer aWriter;
	aWriter.WriteU32(VersusProtocol::VERSION);
	aWriter.WriteU8(sizeof(void*) == 8 ? 64 : 32);
	aWriter.WriteU8(0);
	aWriter.WriteString(mLocalName.empty() ? "Player" : mLocalName);
	mNetwork.SendPacket(VersusProtocol::MSG_HELLO, aWriter.mData);
	mGotHello = true;
}

void VersusSession::SendSimple(uint8_t theType)
{
	mNetwork.SendPacket(theType, std::vector<uint8_t>());
}

void VersusSession::StoreLocalLoadout()
{
	// 选卡界面直接写 mLocal*Seeds；这里只负责拷贝到配置。
}

void VersusSession::SetLocalLoadout(const int* thePlantSeeds, const int* thePlantImitater, const int* theZombieSeeds)
{
	for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
	{
		mLocalPlantSeeds[i] = thePlantSeeds ? thePlantSeeds[i] : -1;
		mLocalPlantImitater[i] = thePlantImitater ? thePlantImitater[i] : -1;
		mLocalZombieSeeds[i] = theZombieSeeds ? theZombieSeeds[i] : -1;
	}
}

bool VersusSession::SendSide(uint8_t theSide)
{
	if (!IsValidSide(theSide) || mRemoteSide == theSide)
	{
		return false;
	}
	mLocalSide = theSide;
	mLocalReady = false;
	VersusProtocol::Writer aWriter;
	aWriter.WriteU8(theSide);
	mNetwork.SendPacket(VersusProtocol::MSG_SIDE, aWriter.mData);
	return true;
}

void VersusSession::ReleaseLocalSide()
{
	mLocalSide = VersusProtocol::SIDE_NONE;
	mLocalReady = false;
	VersusProtocol::Writer aWriter;
	aWriter.WriteU8(VersusProtocol::SIDE_NONE);
	mNetwork.SendPacket(VersusProtocol::MSG_SIDE, aWriter.mData);
}

void VersusSession::ResetSides()
{
	if (!mIsHost)
	{
		return;
	}
	mLocalSide = VersusProtocol::SIDE_NONE;
	mRemoteSide = VersusProtocol::SIDE_NONE;
	mLocalReady = false;
	mRemoteReady = false;
	mNetwork.SendPacket(VersusProtocol::MSG_SIDES_RESET, std::vector<uint8_t>());
}

bool VersusSession::AreSidesReady() const
{
	return IsValidSide(mLocalSide) && IsValidSide(mRemoteSide) && mLocalSide != mRemoteSide;
}

bool VersusSession::CanStartMatch() const
{
	return mIsHost && AreSidesReady() && mLocalReady && mRemoteReady && !mResultReceived;
}

bool VersusSession::SendPick(bool theReady)
{
	if (!AreSidesReady())
	{
		return false;
	}
	mLocalReady = theReady;
	SendPickInternal(theReady);
	return true;
}

void VersusSession::SendPickInternal(bool theReady)
{
	VersusProtocol::Writer aWriter;
	aWriter.WriteU8(mLocalSide);
	for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
	{
		aWriter.WriteI8(static_cast<int8_t>(mLocalSide == VersusProtocol::SIDE_PLANT ? mLocalPlantSeeds[i] : mLocalZombieSeeds[i]));
	}
	for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
	{
		aWriter.WriteI8(static_cast<int8_t>(mLocalSide == VersusProtocol::SIDE_PLANT ? mLocalPlantImitater[i] : -1));
	}
	aWriter.WriteU8(theReady ? 1 : 0);
	mNetwork.SendPacket(VersusProtocol::MSG_PICK, aWriter.mData);
}

bool VersusSession::StartMatch()
{
	if (!CanStartMatch())
	{
		return false;
	}

	std::random_device aRandom;
	uint32_t aSeed = static_cast<uint32_t>(aRandom()) ^ static_cast<uint32_t>(NowMs());
	if (aSeed == 0)
	{
		aSeed = 12345;
	}
	mSeed = aSeed;
	mConfig.mSeed = aSeed;
	mConfig.mHostSide = mIsHost ? mLocalSide : mRemoteSide;
	mConfig.mInputDelay = mInputDelay;
	mConfig.mDurationTicks = VERSUS_MATCH_TICKS;
	mConfig.mStartSun = VERSUS_START_SUN;
	mConfig.mStartBrain = VERSUS_START_BRAIN;
	mConfig.mRulesFlags = 0;

	int8_t aPlantSeeds[VersusProtocol::CARD_SLOTS] = {};
	int8_t aPlantImitater[VersusProtocol::CARD_SLOTS] = {};
	int8_t aZombieSeeds[VersusProtocol::CARD_SLOTS] = {};
	const int* aPlant = (mLocalSide == VersusProtocol::SIDE_PLANT) ? mLocalPlantSeeds : mRemotePlantSeeds;
	const int* aPlantImit = (mLocalSide == VersusProtocol::SIDE_PLANT) ? mLocalPlantImitater : mRemotePlantImitater;
	const int* aZombie = (mLocalSide == VersusProtocol::SIDE_ZOMBIE) ? mLocalZombieSeeds : mRemoteZombieSeeds;
	for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
	{
		aPlantSeeds[i] = static_cast<int8_t>(aPlant[i]);
		aPlantImitater[i] = static_cast<int8_t>(aPlantImit[i]);
		aZombieSeeds[i] = static_cast<int8_t>(aZombie[i]);
	}
	memcpy(mConfig.mPlantSeeds, aPlantSeeds, sizeof(aPlantSeeds));
	memcpy(mConfig.mPlantImitater, aPlantImitater, sizeof(aPlantImitater));
	memcpy(mConfig.mZombieSeeds, aZombieSeeds, sizeof(aZombieSeeds));

	SendStart();
	mConfig.mLocalSide = mLocalSide;
	mStartRequested = true;
	SetPhase(Phase::Loading, "正在创建战场...");
	return true;
}

void VersusSession::SendStart()
{
	VersusProtocol::Writer aWriter;
	aWriter.WriteU32(mConfig.mSeed);
	aWriter.WriteU8(mConfig.mHostSide);
	aWriter.WriteU32(static_cast<uint32_t>(mConfig.mInputDelay));
	aWriter.WriteU32(static_cast<uint32_t>(mConfig.mDurationTicks));
	aWriter.WriteU32(static_cast<uint32_t>(mConfig.mStartSun));
	aWriter.WriteU32(static_cast<uint32_t>(mConfig.mStartBrain));
	for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
	{
		aWriter.WriteI8(mConfig.mPlantSeeds[i]);
	}
	for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
	{
		aWriter.WriteI8(mConfig.mPlantImitater[i]);
	}
	for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
	{
		aWriter.WriteI8(mConfig.mZombieSeeds[i]);
	}
	aWriter.WriteU32(mConfig.mRulesFlags);
	mNetwork.SendPacket(VersusProtocol::MSG_START, aWriter.mData);
}

bool VersusSession::SendLoaded()
{
	mLocalLoaded = true;
	SendSimple(VersusProtocol::MSG_LOADED);
	if (mIsHost && mRemoteLoaded && !mGo)
	{
		mGo = true;
		mGoRequested = true;
		SendSimple(VersusProtocol::MSG_GO);
		SetPhase(Phase::Playing, "对战开始");
	}
	return true;
}

bool VersusSession::TakeStartRequest()
{
	bool aResult = mStartRequested;
	mStartRequested = false;
	return aResult;
}

bool VersusSession::TakeGoRequest()
{
	bool aResult = mGoRequested;
	mGoRequested = false;
	return aResult;
}

void VersusSession::SendInputFrame(const VersusProtocol::InputFrame& theFrame)
{
	std::vector<uint8_t> aPayload;
	if (VersusProtocol::SerializeInputFrame(theFrame, aPayload))
	{
		mNetwork.SendPacket(VersusProtocol::MSG_INPUT, aPayload);
	}
}

void VersusSession::SendChecksum(uint32_t theTick, uint32_t theHash)
{
	VersusProtocol::Writer aWriter;
	aWriter.WriteU32(theTick);
	aWriter.WriteU32(theHash);
	mNetwork.SendPacket(VersusProtocol::MSG_CHECKSUM, aWriter.mData);
}

void VersusSession::SendResult(uint8_t theResult)
{
	if (mResultReceived)
	{
		return;
	}
	mLocalResult = theResult;
	VersusProtocol::Writer aWriter;
	aWriter.WriteU8(theResult);
	mNetwork.SendPacket(VersusProtocol::MSG_RESULT, aWriter.mData);
}

void VersusSession::HandleMessage(uint8_t theType, const uint8_t* thePayload, size_t theSize)
{
	VersusProtocol::Reader aReader(thePayload, theSize);
	switch (theType)
	{
	case VersusProtocol::MSG_HELLO:
	{
		uint32_t aVersion = aReader.ReadU32();
		uint8_t aPointer = aReader.ReadU8();
		aReader.ReadU8();
		std::string aName = aReader.ReadString();
		if (!aReader.IsOk() || aVersion != VersusProtocol::VERSION || aPointer != (sizeof(void*) == 8 ? 64 : 32))
		{
			VersusProtocol::Writer aWriter;
			aWriter.WriteU32(VersusProtocol::VERSION);
			aWriter.WriteU8(0);
			aWriter.WriteString("版本或平台不兼容");
			mNetwork.SendPacket(VersusProtocol::MSG_WELCOME, aWriter.mData);
			SetPhase(Phase::Error, "版本或平台不兼容");
			return;
		}
		mRemoteName = aName;
		VersusProtocol::Writer aWriter;
		aWriter.WriteU32(VersusProtocol::VERSION);
		aWriter.WriteU8(1);
		aWriter.WriteString(mLocalName.empty() ? "Host" : mLocalName);
		mNetwork.SendPacket(VersusProtocol::MSG_WELCOME, aWriter.mData);
		mGotHello = true;
		SetPhase(Phase::Lobby, "对手已连接，请选择阵营");
		break;
	}
	case VersusProtocol::MSG_WELCOME:
	{
		uint32_t aVersion = aReader.ReadU32();
		uint8_t aAccepted = aReader.ReadU8();
		std::string aName = aReader.ReadString();
		if (!aReader.IsOk() || aVersion != VersusProtocol::VERSION || !aAccepted)
		{
			SetPhase(Phase::Error, aName.empty() ? "主机拒绝连接" : aName);
			return;
		}
		mRemoteName = aName;
		SetPhase(Phase::Lobby, "已连接，请选择阵营");
		break;
	}
	case VersusProtocol::MSG_SIDE:
	{
		uint8_t aSide = aReader.ReadU8();
		if (!aReader.IsOk())
		{
			break;
		}
		if (aSide == VersusProtocol::SIDE_NONE)
		{
			mRemoteSide = VersusProtocol::SIDE_NONE;
			mRemoteReady = false;
			break;
		}
		if (aSide == mLocalSide)
		{
			// 双方同时选了同一阵营：房主优先，给客机分配另一边。
			if (mIsHost)
			{
				mRemoteSide = OppositeSide(aSide);
				mRemoteReady = false;
				VersusProtocol::Writer aWriter;
				aWriter.WriteU8(OppositeSide(aSide));
				mNetwork.SendPacket(VersusProtocol::MSG_SIDE_ASSIGN, aWriter.mData);
				mStatus = "双方选择了同一阵营，已为对手分配另一方";
			}
			else
			{
				mLocalSide = VersusProtocol::SIDE_NONE;
				mLocalReady = false;
				mStatus = "双方选择了同一阵营，等待房主分配";
			}
			break;
		}
		if (IsValidSide(aSide) && (mLocalSide == VersusProtocol::SIDE_NONE || mLocalSide != aSide))
		{
			mRemoteSide = aSide;
			mLocalReady = false;
			mRemoteReady = false;
		}
		break;
	}
	case VersusProtocol::MSG_SIDE_ASSIGN:
	{
		uint8_t aSide = aReader.ReadU8();
		if (aReader.IsOk() && IsValidSide(aSide) && !mIsHost)
		{
			mLocalSide = aSide;
			mLocalReady = false;
			mRemoteSide = OppositeSide(aSide);
			mRemoteReady = false;
			VersusProtocol::Writer aWriter;
			aWriter.WriteU8(aSide);
			mNetwork.SendPacket(VersusProtocol::MSG_SIDE, aWriter.mData);
			mStatus = "房主已分配阵营，请选卡";
		}
		break;
	}
	case VersusProtocol::MSG_SIDES_RESET:
		mLocalSide = VersusProtocol::SIDE_NONE;
		mRemoteSide = VersusProtocol::SIDE_NONE;
		mLocalReady = false;
		mRemoteReady = false;
		break;
	case VersusProtocol::MSG_PICK:
	{
		mRemoteSide = aReader.ReadU8();
		for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
		{
			int8_t aSeed = aReader.ReadI8();
			if (mRemoteSide == VersusProtocol::SIDE_PLANT)
			{
				mRemotePlantSeeds[i] = aSeed;
			}
			else
			{
				mRemoteZombieSeeds[i] = aSeed;
			}
		}
		for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
		{
			int8_t aImitater = aReader.ReadI8();
			if (mRemoteSide == VersusProtocol::SIDE_PLANT)
			{
				mRemotePlantImitater[i] = aImitater;
			}
		}
		mRemoteReady = aReader.ReadU8() != 0;
		break;
	}
	case VersusProtocol::MSG_START:
	{
		mConfig.mSeed = aReader.ReadU32();
		mConfig.mHostSide = aReader.ReadU8();
		mConfig.mInputDelay = static_cast<int32_t>(aReader.ReadU32());
		mConfig.mDurationTicks = static_cast<int32_t>(aReader.ReadU32());
		mConfig.mStartSun = static_cast<int32_t>(aReader.ReadU32());
		mConfig.mStartBrain = static_cast<int32_t>(aReader.ReadU32());
		for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
		{
			mConfig.mPlantSeeds[i] = aReader.ReadI8();
		}
		for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
		{
			mConfig.mPlantImitater[i] = aReader.ReadI8();
		}
		for (int i = 0; i < VersusProtocol::CARD_SLOTS; i++)
		{
			mConfig.mZombieSeeds[i] = aReader.ReadI8();
		}
		mConfig.mRulesFlags = aReader.ReadU32();
		if (!aReader.IsOk())
		{
			SetPhase(Phase::Error, "收到损坏的开局配置");
			return;
		}
		mConfig.mLocalSide = OppositeSide(mConfig.mHostSide);
		mStartRequested = true;
		SetPhase(Phase::Loading, "正在创建战场...");
		break;
	}
	case VersusProtocol::MSG_LOADED:
		mRemoteLoaded = true;
		if (mIsHost && mLocalLoaded && !mGo)
		{
			mGo = true;
			mGoRequested = true;
			SendSimple(VersusProtocol::MSG_GO);
			SetPhase(Phase::Playing, "对战开始");
		}
		break;
	case VersusProtocol::MSG_GO:
		mGo = true;
		mGoRequested = true;
		SetPhase(Phase::Playing, "对战开始");
		break;
	case VersusProtocol::MSG_INPUT:
	{
		VersusProtocol::InputFrame aFrame;
		if (VersusProtocol::DeserializeInputFrame(thePayload, theSize, aFrame))
		{
			mRemoteFrames[aFrame.mTick] = aFrame;
		}
		break;
	}
	case VersusProtocol::MSG_CHECKSUM:
	{
		uint32_t aTick = aReader.ReadU32();
		uint32_t aHash = aReader.ReadU32();
		if (aReader.IsOk())
		{
			mRemoteChecksums[aTick] = aHash;
		}
		break;
	}
	case VersusProtocol::MSG_RESULT:
	{
		uint8_t aResult = aReader.ReadU8();
		if (aReader.IsOk() && !mResultReceived)
		{
			mResultReceived = true;
			mResult = aResult;
			SetPhase(Phase::Result, aResult == VersusProtocol::RESULT_PLANT_WIN ? "植物方胜利" : "僵尸方胜利");
		}
		break;
	}
	case VersusProtocol::MSG_PING:
	{
		VersusProtocol::Writer aWriter;
		aWriter.WriteU32(0);
		mNetwork.SendPacket(VersusProtocol::MSG_PONG, aWriter.mData);
		break;
	}
	case VersusProtocol::MSG_PONG:
		if (mPingSentAt != 0)
		{
			mPingMs = static_cast<int32_t>(NowMs() - mPingSentAt);
			mPingSentAt = 0;
		}
		break;
	case VersusProtocol::MSG_BYE:
		if (!mResultReceived)
		{
			mResultReceived = true;
			mResult = mLocalSide;
			SetPhase(Phase::Result, "对手已离开，你获胜");
		}
		break;
	case VersusProtocol::MSG_ERROR:
	{
		std::string aError = aReader.ReadString();
		SetPhase(Phase::Error, aError);
		break;
	}
	default:
		break;
	}
}

bool VersusSession::HasRemoteFrame(uint32_t theTick) const
{
	return mRemoteFrames.find(theTick) != mRemoteFrames.end();
}

const VersusProtocol::InputFrame* VersusSession::GetRemoteFrame(uint32_t theTick) const
{
	auto aIt = mRemoteFrames.find(theTick);
	return aIt == mRemoteFrames.end() ? nullptr : &aIt->second;
}

bool VersusSession::HasRemoteChecksum(uint32_t theTick) const
{
	return mRemoteChecksums.find(theTick) != mRemoteChecksums.end();
}

uint32_t VersusSession::GetRemoteChecksum(uint32_t theTick) const
{
	auto aIt = mRemoteChecksums.find(theTick);
	return aIt == mRemoteChecksums.end() ? 0 : aIt->second;
}
