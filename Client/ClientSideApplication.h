#pragma once

#include <deque>
#include <vector>
#include <mutex>
#include <thread>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <functional>
#include "../SocketsClientServer/SocketCreator.h"
#pragma lib("SocketCreator.lib")

class ClientSideApplication : public SocketCreator
{
public:
	ClientSideApplication();
	virtual void ListenForMessage() final;
	void Wait() noexcept;
private:
	std::mutex lock;
	std::thread listener;
};

