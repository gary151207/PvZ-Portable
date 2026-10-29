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

#ifndef __VERSUSNETWORK_H__
#define __VERSUSNETWORK_H__

#include <cstdint>
#include <string>
#include <vector>

// +Gaga: 局域网 TCP 传输层。非阻塞、单线程轮询。
class VersusNetwork
{
public:
	enum class State
	{
		Disconnected,
		Listening,
		Connecting,
		Connected,
		Error
	};

	VersusNetwork();
	~VersusNetwork();

	static bool IsPlatformSupported();
	static std::string GetLocalIPv4();

	bool Host(uint16_t thePort, std::string& theError);
	bool Join(const std::string& theHost, uint16_t thePort, std::string& theError);
	void Close();

	void Update();
	bool IsConnected() const;
	bool IsListening() const;
	State GetState() const { return mState; }
	const std::string& GetError() const { return mError; }
	int GetRttMs() const { return mRttMs; }

	bool SendPacket(uint8_t theType, const std::vector<uint8_t>& thePayload);
	bool PollMessage(std::vector<uint8_t>& theMessage);

private:
	void FlushSendBuffer();
	void PumpReceive();
	void SetError(const std::string& theError);

#if defined(_WIN32)
	uintptr_t mSocket;
	uintptr_t mListenSocket;
#else
	int mSocket;
	int mListenSocket;
#endif
	State mState;
	std::string mError;
	std::vector<uint8_t> mSendBuffer;
	std::vector<uint8_t> mReceiveBuffer;
	std::vector<std::vector<uint8_t>> mMessages;
	uint64_t mLastReceiveTime;
	uint64_t mConnectStartTime;
	int mRttMs;
	bool mWsaStarted;

	void CloseSocket();
};

#endif
