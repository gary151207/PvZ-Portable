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

#include "VersusNetwork.h"
#include "VersusProtocol.h"

#include <cerrno>
#include <chrono>
#include <cstring>

#if defined(_WIN32)
#	include <winsock2.h>
#	include <ws2tcpip.h>
#	pragma comment(lib, "ws2_32.lib")
using SocketType = SOCKET;
using SockLenType = int;
static const SocketType INVALID_SOCK = INVALID_SOCKET;
#else
#	include <arpa/inet.h>
#	include <fcntl.h>
#	include <netdb.h>
#	include <netinet/in.h>
#	include <netinet/tcp.h>
#	include <sys/select.h>
#	include <sys/socket.h>
#	include <unistd.h>
using SocketType = int;
using SockLenType = socklen_t;
static const SocketType INVALID_SOCK = -1;
#endif

namespace
{
	uint64_t NowMs()
	{
		using namespace std::chrono;
		return static_cast<uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
	}

	bool SetNonBlocking(SocketType theSocket)
	{
#if defined(_WIN32)
		u_long aMode = 1;
		return ioctlsocket(theSocket, FIONBIO, &aMode) == 0;
#else
		int aFlags = fcntl(theSocket, F_GETFL, 0);
		if (aFlags < 0)
		{
			return false;
		}
		return fcntl(theSocket, F_SETFL, aFlags | O_NONBLOCK) == 0;
#endif
	}

	void SetNoDelay(SocketType theSocket)
	{
		int aYes = 1;
		setsockopt(theSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&aYes), sizeof(aYes));
#if defined(SO_NOSIGPIPE)
		setsockopt(theSocket, SOL_SOCKET, SO_NOSIGPIPE, reinterpret_cast<const char*>(&aYes), sizeof(aYes));
#endif
	}

	int LastSocketError()
	{
#if defined(_WIN32)
		return WSAGetLastError();
#else
		return errno;
#endif
	}

	bool IsWouldBlock(int theError)
	{
#if defined(_WIN32)
		return theError == WSAEWOULDBLOCK || theError == WSAEINPROGRESS;
#else
		return theError == EWOULDBLOCK || theError == EAGAIN || theError == EINPROGRESS;
#endif
	}

	std::string ErrorString(int theError)
	{
#if defined(_WIN32)
		char aBuffer[256] = {};
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, theError, 0, aBuffer, sizeof(aBuffer), nullptr);
		return std::string(aBuffer);
#else
		return std::string(strerror(theError));
#endif
	}
}

VersusNetwork::VersusNetwork()
{
	mSocket = INVALID_SOCK;
	mListenSocket = INVALID_SOCK;
	mState = State::Disconnected;
	mRttMs = 0;
	mLastReceiveTime = NowMs();
	mConnectStartTime = 0;
	mWsaStarted = false;
#if defined(_WIN32)
	WSADATA aData;
	if (WSAStartup(MAKEWORD(2, 2), &aData) == 0)
	{
		mWsaStarted = true;
	}
#endif
}

VersusNetwork::~VersusNetwork()
{
	Close();
#if defined(_WIN32)
	if (mWsaStarted)
	{
		WSACleanup();
	}
#endif
}

bool VersusNetwork::IsPlatformSupported()
{
#if defined(__EMSCRIPTEN__) || defined(NINTENDO_SWITCH) || defined(NINTENDO_3DS)
	return false;
#else
	return true;
#endif
}

std::string VersusNetwork::GetLocalIPv4()
{
	char aHostName[256] = {};
	if (gethostname(aHostName, sizeof(aHostName)) != 0)
	{
		return "127.0.0.1";
	}

	addrinfo aHints = {};
	aHints.ai_family = AF_INET;
	aHints.ai_socktype = SOCK_STREAM;
	addrinfo* aResult = nullptr;
	if (getaddrinfo(aHostName, nullptr, &aHints, &aResult) != 0)
	{
		return "127.0.0.1";
	}

	std::string aFallback;
	for (addrinfo* aIt = aResult; aIt != nullptr; aIt = aIt->ai_next)
	{
		if (aIt->ai_family != AF_INET)
		{
			continue;
		}
		char aBuffer[INET_ADDRSTRLEN] = {};
		sockaddr_in* aAddress = reinterpret_cast<sockaddr_in*>(aIt->ai_addr);
		if (inet_ntop(AF_INET, &aAddress->sin_addr, aBuffer, sizeof(aBuffer)) == nullptr)
		{
			continue;
		}
		std::string aIp(aBuffer);
		if (aIp != "127.0.0.1" && aIp != "0.0.0.0")
		{
			freeaddrinfo(aResult);
			return aIp;
		}
		if (aFallback.empty())
		{
			aFallback = aIp;
		}
	}
	freeaddrinfo(aResult);
	return aFallback.empty() ? "127.0.0.1" : aFallback;
}

void VersusNetwork::CloseSocket()
{
#if defined(_WIN32)
	if (mSocket != INVALID_SOCK)
	{
		closesocket(static_cast<SOCKET>(mSocket));
		mSocket = INVALID_SOCK;
	}
	if (mListenSocket != INVALID_SOCK)
	{
		closesocket(static_cast<SOCKET>(mListenSocket));
		mListenSocket = INVALID_SOCK;
	}
#else
	if (mSocket != INVALID_SOCK)
	{
		close(mSocket);
		mSocket = INVALID_SOCK;
	}
	if (mListenSocket != INVALID_SOCK)
	{
		close(mListenSocket);
		mListenSocket = INVALID_SOCK;
	}
#endif
}

void VersusNetwork::Close()
{
	CloseSocket();
	mState = State::Disconnected;
	mSendBuffer.clear();
	mReceiveBuffer.clear();
	mMessages.clear();
	mError.clear();
}

void VersusNetwork::SetError(const std::string& theError)
{
	mError = theError;
	mState = State::Error;
	CloseSocket();
}

bool VersusNetwork::Host(uint16_t thePort, std::string& theError)
{
	Close();
	theError.clear();
	if (!IsPlatformSupported())
	{
		theError = "该平台不支持联机";
		return false;
	}

	SocketType aSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (aSocket == INVALID_SOCK)
	{
		theError = "创建 socket 失败: " + ErrorString(LastSocketError());
		return false;
	}

	int aReuse = 1;
	setsockopt(aSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&aReuse), sizeof(aReuse));

	sockaddr_in aAddress = {};
	aAddress.sin_family = AF_INET;
	aAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	aAddress.sin_port = htons(thePort);
	if (bind(aSocket, reinterpret_cast<sockaddr*>(&aAddress), sizeof(aAddress)) != 0)
	{
		theError = "端口被占用或无法监听: " + ErrorString(LastSocketError());
#if defined(_WIN32)
		closesocket(aSocket);
#else
		close(aSocket);
#endif
		return false;
	}
	if (listen(aSocket, 1) != 0)
	{
		theError = "监听失败: " + ErrorString(LastSocketError());
#if defined(_WIN32)
		closesocket(aSocket);
#else
		close(aSocket);
#endif
		return false;
	}
	if (!SetNonBlocking(aSocket))
	{
		theError = "设置非阻塞失败";
#if defined(_WIN32)
		closesocket(aSocket);
#else
		close(aSocket);
#endif
		return false;
	}

#if defined(_WIN32)
	mListenSocket = reinterpret_cast<uintptr_t>(aSocket);
#else
	mListenSocket = aSocket;
#endif
	mState = State::Listening;
	mLastReceiveTime = NowMs();
	return true;
}

bool VersusNetwork::Join(const std::string& theHost, uint16_t thePort, std::string& theError)
{
	Close();
	theError.clear();
	if (!IsPlatformSupported())
	{
		theError = "该平台不支持联机";
		return false;
	}

	std::string aHost = theHost;
	uint16_t aPort = thePort;
	size_t aColon = aHost.find_last_of(':');
	if (aColon != std::string::npos && aHost.find(':') == aColon)
	{
		std::string aPortText = aHost.substr(aColon + 1);
		if (!aPortText.empty())
		{
			int aParsedPort = atoi(aPortText.c_str());
			if (aParsedPort > 0 && aParsedPort < 65536)
			{
				aPort = static_cast<uint16_t>(aParsedPort);
				aHost = aHost.substr(0, aColon);
			}
		}
	}
	if (aHost.empty())
	{
		theError = "请输入主机 IP";
		return false;
	}

	addrinfo aHints = {};
	aHints.ai_family = AF_INET;
	aHints.ai_socktype = SOCK_STREAM;
	addrinfo* aResult = nullptr;
	std::string aPortText = std::to_string(aPort);
	if (getaddrinfo(aHost.c_str(), aPortText.c_str(), &aHints, &aResult) != 0)
	{
		theError = "无法解析主机地址";
		return false;
	}

	SocketType aSocket = INVALID_SOCK;
	for (addrinfo* aIt = aResult; aIt != nullptr; aIt = aIt->ai_next)
	{
		aSocket = socket(aIt->ai_family, aIt->ai_socktype, aIt->ai_protocol);
		if (aSocket == INVALID_SOCK)
		{
			continue;
		}
		if (!SetNonBlocking(aSocket))
		{
#if defined(_WIN32)
			closesocket(aSocket);
#else
			close(aSocket);
#endif
			aSocket = INVALID_SOCK;
			continue;
		}
		int aResultCode = connect(aSocket, aIt->ai_addr, static_cast<int>(aIt->ai_addrlen));
		if (aResultCode == 0)
		{
			break;
		}
		int aError = LastSocketError();
		if (!IsWouldBlock(aError))
		{
#if defined(_WIN32)
			closesocket(aSocket);
#else
			close(aSocket);
#endif
			aSocket = INVALID_SOCK;
			continue;
		}
		break;
	}
	freeaddrinfo(aResult);

	if (aSocket == INVALID_SOCK)
	{
		theError = "连接失败";
		return false;
	}

	SetNoDelay(aSocket);
#if defined(_WIN32)
	mSocket = reinterpret_cast<uintptr_t>(aSocket);
#else
	mSocket = aSocket;
#endif
	mState = State::Connecting;
	mLastReceiveTime = NowMs();
	mConnectStartTime = NowMs();
	return true;
}

void VersusNetwork::Update()
{
	if (mState == State::Listening)
	{
		if (mListenSocket == INVALID_SOCK)
		{
			return;
		}
		sockaddr_in aAddress = {};
		SockLenType aLength = sizeof(aAddress);
		SocketType aClient = accept(static_cast<SocketType>(mListenSocket), reinterpret_cast<sockaddr*>(&aAddress), &aLength);
		if (aClient != INVALID_SOCK)
		{
			SetNonBlocking(aClient);
			SetNoDelay(aClient);
			// 主机只接受一名客人，接受后关闭监听 socket，但保留客户端 socket。
#if defined(_WIN32)
			closesocket(static_cast<SOCKET>(mListenSocket));
			mListenSocket = INVALID_SOCK;
			mSocket = reinterpret_cast<uintptr_t>(aClient);
#else
			close(mListenSocket);
			mListenSocket = INVALID_SOCK;
			mSocket = aClient;
#endif
			mState = State::Connected;
			mLastReceiveTime = NowMs();
		}
		return;
	}

	if (mState == State::Connecting)
	{
		fd_set aWriteSet;
		FD_ZERO(&aWriteSet);
		FD_SET(static_cast<SocketType>(mSocket), &aWriteSet);
		timeval aTimeout = { 0, 0 };
		int aResult = select(static_cast<int>(mSocket) + 1, nullptr, &aWriteSet, nullptr, &aTimeout);
		if (aResult > 0)
		{
			int aError = 0;
			SockLenType aLength = sizeof(aError);
			getsockopt(static_cast<SocketType>(mSocket), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&aError), &aLength);
			if (aError == 0)
			{
				mState = State::Connected;
				mLastReceiveTime = NowMs();
			}
			else
			{
				SetError("连接失败: " + ErrorString(aError));
			}
		}
		else if (aResult < 0 && !IsWouldBlock(LastSocketError()))
		{
			SetError("连接失败: " + ErrorString(LastSocketError()));
		}
		return;
	}

	if (mState != State::Connected)
	{
		return;
	}

	PumpReceive();
	FlushSendBuffer();

	if (NowMs() - mLastReceiveTime > 15000)
	{
		SetError("连接超时");
	}
}

void VersusNetwork::PumpReceive()
{
	if (mSocket == INVALID_SOCK)
	{
		return;
	}
	uint8_t aBuffer[4096];
	for (;;)
	{
		int aRead = recv(static_cast<SocketType>(mSocket), reinterpret_cast<char*>(aBuffer), sizeof(aBuffer), 0);
		if (aRead > 0)
		{
			mLastReceiveTime = NowMs();
			mReceiveBuffer.insert(mReceiveBuffer.end(), aBuffer, aBuffer + aRead);
			continue;
		}
		if (aRead == 0)
		{
			SetError("对方已断开连接");
			return;
		}
		int aError = LastSocketError();
		if (!IsWouldBlock(aError))
		{
			SetError("接收失败: " + ErrorString(aError));
		}
		break;
	}

	for (;;)
	{
		if (mReceiveBuffer.size() < 4)
		{
			break;
		}
		uint32_t aLength = static_cast<uint32_t>(mReceiveBuffer[0])
			| (static_cast<uint32_t>(mReceiveBuffer[1]) << 8)
			| (static_cast<uint32_t>(mReceiveBuffer[2]) << 16)
			| (static_cast<uint32_t>(mReceiveBuffer[3]) << 24);
		if (aLength < 1 || aLength > VersusProtocol::MAX_MESSAGE_SIZE)
		{
			SetError("收到非法网络消息");
			return;
		}
		if (mReceiveBuffer.size() < aLength + 4)
		{
			break;
		}
		std::vector<uint8_t> aMessage(mReceiveBuffer.begin(), mReceiveBuffer.begin() + aLength + 4);
		mReceiveBuffer.erase(mReceiveBuffer.begin(), mReceiveBuffer.begin() + aLength + 4);
		mMessages.push_back(std::move(aMessage));
	}
}

void VersusNetwork::FlushSendBuffer()
{
	if (mSocket == INVALID_SOCK || mSendBuffer.empty())
	{
		return;
	}
	size_t aSentTotal = 0;
	while (aSentTotal < mSendBuffer.size())
	{
#if defined(_WIN32)
		int aSent = send(static_cast<SocketType>(mSocket), reinterpret_cast<const char*>(mSendBuffer.data() + aSentTotal), static_cast<int>(mSendBuffer.size() - aSentTotal), 0);
#elif defined(MSG_NOSIGNAL)
		int aSent = send(static_cast<SocketType>(mSocket), mSendBuffer.data() + aSentTotal, mSendBuffer.size() - aSentTotal, MSG_NOSIGNAL);
#else
		int aSent = send(static_cast<SocketType>(mSocket), mSendBuffer.data() + aSentTotal, mSendBuffer.size() - aSentTotal, 0);
#endif
		if (aSent > 0)
		{
			aSentTotal += static_cast<size_t>(aSent);
			continue;
		}
		if (aSent == 0)
		{
			break;
		}
		int aError = LastSocketError();
		if (!IsWouldBlock(aError))
		{
			SetError("发送失败: " + ErrorString(aError));
		}
		break;
	}
	if (aSentTotal > 0)
	{
		mSendBuffer.erase(mSendBuffer.begin(), mSendBuffer.begin() + aSentTotal);
	}
}

bool VersusNetwork::SendPacket(uint8_t theType, const std::vector<uint8_t>& thePayload)
{
	if (mState != State::Connected)
	{
		return false;
	}
	std::vector<uint8_t> aMessage = VersusProtocol::MakeMessage(theType, thePayload);
	mSendBuffer.insert(mSendBuffer.end(), aMessage.begin(), aMessage.end());
	FlushSendBuffer();
	return true;
}

bool VersusNetwork::PollMessage(std::vector<uint8_t>& theMessage)
{
	if (mMessages.empty())
	{
		return false;
	}
	theMessage = std::move(mMessages.front());
	mMessages.erase(mMessages.begin());
	return true;
}

bool VersusNetwork::IsConnected() const
{
	return mState == State::Connected;
}

bool VersusNetwork::IsListening() const
{
	return mState == State::Listening;
}
