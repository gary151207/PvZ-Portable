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

#include "VersusProtocol.h"

#include <cstring>

namespace VersusProtocol
{
	void Writer::WriteU8(uint8_t theValue)
	{
		mData.push_back(theValue);
	}

	void Writer::WriteI8(int8_t theValue)
	{
		mData.push_back(static_cast<uint8_t>(theValue));
	}

	void Writer::WriteU16(uint16_t theValue)
	{
		mData.push_back(static_cast<uint8_t>(theValue & 0xFF));
		mData.push_back(static_cast<uint8_t>((theValue >> 8) & 0xFF));
	}

	void Writer::WriteU32(uint32_t theValue)
	{
		for (int i = 0; i < 4; i++)
		{
			mData.push_back(static_cast<uint8_t>((theValue >> (i * 8)) & 0xFF));
		}
	}

	void Writer::WriteString(const std::string& theValue)
	{
		// 网络字符串只用于玩家名，长度限制在 255 字节以内。
		uint16_t aLength = static_cast<uint16_t>(theValue.size() > 255 ? 255 : theValue.size());
		WriteU16(aLength);
		WriteBytes(theValue.data(), aLength);
	}

	void Writer::WriteBytes(const void* theData, size_t theSize)
	{
		const uint8_t* aData = static_cast<const uint8_t*>(theData);
		mData.insert(mData.end(), aData, aData + theSize);
	}

	Reader::Reader(const uint8_t* theData, size_t theSize)
	{
		mData = theData;
		mSize = theSize;
	}

	uint8_t Reader::ReadU8()
	{
		if (mPos + 1 > mSize)
		{
			mError = true;
			return 0;
		}
		return mData[mPos++];
	}

	int8_t Reader::ReadI8()
	{
		return static_cast<int8_t>(ReadU8());
	}

	uint16_t Reader::ReadU16()
	{
		if (mPos + 2 > mSize)
		{
			mError = true;
			return 0;
		}
		uint16_t aValue = static_cast<uint16_t>(mData[mPos]) | (static_cast<uint16_t>(mData[mPos + 1]) << 8);
		mPos += 2;
		return aValue;
	}

	uint32_t Reader::ReadU32()
	{
		if (mPos + 4 > mSize)
		{
			mError = true;
			return 0;
		}
		uint32_t aValue = 0;
		for (int i = 0; i < 4; i++)
		{
			aValue |= static_cast<uint32_t>(mData[mPos + i]) << (i * 8);
		}
		mPos += 4;
		return aValue;
	}

	std::string Reader::ReadString()
	{
		uint16_t aLength = ReadU16();
		if (mPos + aLength > mSize)
		{
			mError = true;
			return std::string();
		}
		std::string aValue(reinterpret_cast<const char*>(mData + mPos), aLength);
		mPos += aLength;
		return aValue;
	}

	void Reader::ReadBytes(void* theData, size_t theSize)
	{
		if (mPos + theSize > mSize)
		{
			mError = true;
			return;
		}
		memcpy(theData, mData + mPos, theSize);
		mPos += theSize;
	}

	std::vector<uint8_t> MakeMessage(uint8_t theType, const std::vector<uint8_t>& thePayload)
	{
		std::vector<uint8_t> aMessage;
		uint32_t aLength = static_cast<uint32_t>(thePayload.size() + 1);
		aMessage.reserve(aLength + 4);
		aMessage.push_back(static_cast<uint8_t>(aLength & 0xFF));
		aMessage.push_back(static_cast<uint8_t>((aLength >> 8) & 0xFF));
		aMessage.push_back(static_cast<uint8_t>((aLength >> 16) & 0xFF));
		aMessage.push_back(static_cast<uint8_t>((aLength >> 24) & 0xFF));
		aMessage.push_back(theType);
		aMessage.insert(aMessage.end(), thePayload.begin(), thePayload.end());
		return aMessage;
	}

	bool ParseMessage(const std::vector<uint8_t>& theMessage, uint8_t& theType, const uint8_t*& thePayload, size_t& thePayloadSize)
	{
		if (theMessage.size() < 5)
		{
			return false;
		}
		uint32_t aLength = static_cast<uint32_t>(theMessage[0])
			| (static_cast<uint32_t>(theMessage[1]) << 8)
			| (static_cast<uint32_t>(theMessage[2]) << 16)
			| (static_cast<uint32_t>(theMessage[3]) << 24);
		if (aLength < 1 || aLength > MAX_MESSAGE_SIZE || aLength + 4 != theMessage.size())
		{
			return false;
		}
		theType = theMessage[4];
		thePayload = theMessage.data() + 5;
		thePayloadSize = aLength - 1;
		return true;
	}

	bool SerializeInputFrame(const InputFrame& theFrame, std::vector<uint8_t>& thePayload)
	{
		if (theFrame.mCommands.size() > MAX_COMMANDS_PER_FRAME)
		{
			return false;
		}
		Writer aWriter;
		aWriter.WriteU32(theFrame.mTick);
		aWriter.WriteU8(static_cast<uint8_t>(theFrame.mCommands.size()));
		for (const Command& aCommand : theFrame.mCommands)
		{
			aWriter.WriteU8(aCommand.mType);
			aWriter.WriteI8(aCommand.mSeedType);
			aWriter.WriteI8(aCommand.mImitaterType);
			aWriter.WriteU8(aCommand.mGridX);
			aWriter.WriteU8(aCommand.mGridY);
		}
		thePayload = std::move(aWriter.mData);
		return true;
	}

	bool DeserializeInputFrame(const uint8_t* theData, size_t theSize, InputFrame& theFrame)
	{
		Reader aReader(theData, theSize);
		theFrame.mTick = aReader.ReadU32();
		uint8_t aCount = aReader.ReadU8();
		if (!aReader.IsOk() || aCount > MAX_COMMANDS_PER_FRAME)
		{
			return false;
		}
		theFrame.mCommands.clear();
		theFrame.mCommands.reserve(aCount);
		for (int i = 0; i < aCount; i++)
		{
			Command aCommand;
			aCommand.mType = aReader.ReadU8();
			aCommand.mSeedType = aReader.ReadI8();
			aCommand.mImitaterType = aReader.ReadI8();
			aCommand.mGridX = aReader.ReadU8();
			aCommand.mGridY = aReader.ReadU8();
			if (aCommand.mType < CMD_PLANT || aCommand.mType > CMD_SHOVEL)
			{
				return false;
			}
			theFrame.mCommands.push_back(aCommand);
		}
		return aReader.IsOk() && aReader.Remaining() == 0;
	}
}
