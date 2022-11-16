#pragma once

#include <WS2tcpip.h>
#include <WinSock2.h>
#include <string>
#include <iostream>
#include <tchar.h>
#include <functional>

constexpr int PORT = 55555;
const std::string address = "127.0.0.1";
constexpr unsigned int MAX_BUFFER_SIZE = 4096;

// Wanna be interface
class SocketCreator
{
public:
	virtual inline int GetPort() const noexcept final { return PORT; };
	virtual inline std::string GetAddress()  const noexcept final { return address; };
	SocketCreator(bool isServer);
	virtual int Send(SOCKET socket, const void const* object, int objectSize) final;
	virtual int Receive(SOCKET socket, int additionalBytes, const std::function<void(void*, int bytesReceived)>& handleObject) final;
	virtual void CloseSocket() const noexcept;
protected:
	void ConnectSocket(bool isHost) noexcept;
	void StartUpSocket() noexcept;
	virtual void DoCleanup() noexcept;
	SOCKET mainSocket;
private:
	void CreateMainSocket() noexcept;
};

