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

#ifndef __VERSUSPROTOCOL_H__
#define __VERSUSPROTOCOL_H__

#include <cstdint>
#include <string>
#include <vector>

// +Gaga: 本地联机对战的网络协议。只负责字节流，不依赖游戏对象。
namespace VersusProtocol
{
	constexpr uint32_t VERSION = 1;
	constexpr int MAX_COMMANDS_PER_FRAME = 8;
	constexpr int MAX_MESSAGE_SIZE = 64 * 1024;
	constexpr int CARD_SLOTS = 6;

	enum MessageType : uint8_t
	{
		MSG_HELLO = 1,
		MSG_WELCOME,
		MSG_SIDE,
		MSG_SIDES_RESET,
		MSG_PICK,
		MSG_START,
		MSG_LOADED,
		MSG_GO,
		MSG_INPUT,
		MSG_CHECKSUM,
		MSG_RESULT,
		MSG_PING,
		MSG_PONG,
		MSG_BYE,
		MSG_ERROR,
		MSG_SIDE_ASSIGN
	};

	enum Side : uint8_t
	{
		SIDE_NONE = 0,
		SIDE_PLANT = 1,
		SIDE_ZOMBIE = 2
	};

	enum Result : uint8_t
	{
		RESULT_NONE = 0,
		RESULT_PLANT_WIN,
		RESULT_ZOMBIE_WIN,
		RESULT_ABORT,
		RESULT_FORFEIT
	};

	enum CommandType : uint8_t
	{
		CMD_PLANT = 1,
		CMD_ZOMBIE,
		CMD_SHOVEL
	};

	struct Command
	{
		uint8_t mType = 0;
		int8_t mSeedType = -1;
		int8_t mImitaterType = -1;
		uint8_t mGridX = 0;
		uint8_t mGridY = 0;
	};

	struct InputFrame
	{
		uint32_t mTick = 0;
		std::vector<Command> mCommands;
	};

	struct MatchConfig
	{
		uint32_t mSeed = 0;
		uint8_t mLocalSide = SIDE_NONE;
		uint8_t mHostSide = SIDE_NONE;
		int32_t mInputDelay = 4;
		int32_t mDurationTicks = 18000;
		int32_t mStartSun = 150;
		int32_t mStartBrain = 150;
		int8_t mPlantSeeds[CARD_SLOTS] = {};
		int8_t mPlantImitater[CARD_SLOTS] = {};
		int8_t mZombieSeeds[CARD_SLOTS] = {};
		uint32_t mRulesFlags = 0;
	};

	class Writer
	{
	public:
		std::vector<uint8_t> mData;

		void WriteU8(uint8_t theValue);
		void WriteI8(int8_t theValue);
		void WriteU16(uint16_t theValue);
		void WriteU32(uint32_t theValue);
		void WriteString(const std::string& theValue);
		void WriteBytes(const void* theData, size_t theSize);
	};

	class Reader
	{
	public:
		const uint8_t* mData = nullptr;
		size_t mSize = 0;
		size_t mPos = 0;
		bool mError = false;

		Reader(const uint8_t* theData, size_t theSize);
		uint8_t ReadU8();
		int8_t ReadI8();
		uint16_t ReadU16();
		uint32_t ReadU32();
		std::string ReadString();
		void ReadBytes(void* theData, size_t theSize);
		bool IsOk() const { return !mError; }
		size_t Remaining() const { return mSize - mPos; }
	};

	// 消息头：4 字节长度（不含长度本身）+ 1 字节类型。
	// MakeMessage 会在头部写入长度；ParseMessage 只返回类型和载荷。
	std::vector<uint8_t> MakeMessage(uint8_t theType, const std::vector<uint8_t>& thePayload);
	bool ParseMessage(const std::vector<uint8_t>& theMessage, uint8_t& theType, const uint8_t*& thePayload, size_t& thePayloadSize);

	bool SerializeInputFrame(const InputFrame& theFrame, std::vector<uint8_t>& thePayload);
	bool DeserializeInputFrame(const uint8_t* theData, size_t theSize, InputFrame& theFrame);
}

#endif
