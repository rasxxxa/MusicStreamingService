#pragma once
#include <deque>
#include <vector>
#include <mutex>
#include <thread>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include "../SocketsClientServer/SocketCreator.h"

#pragma lib("SocketCreator.lib")

class ServerSideApplication : public SocketCreator
{
private:
	std::mutex lock;
	std::thread listener;
	std::vector<std::thread> listeners;
	std::vector<SOCKET> acceptSockets;

	void ListenForSockets();
	void ListenForMessages(SOCKET& socket);
	void InitializeServerApplication();
protected:

public:
	ServerSideApplication();
	void Wait() noexcept;
};

