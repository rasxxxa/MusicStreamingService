#pragma once
#include <deque>
#include <vector>
#include <mutex>
#include <thread>
#include <string>
#include "../OpenAL/Sound.h"
#include <atomic>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include "../SocketsClientServer/SocketCreator.h"


class ServerSideApplication : public SocketCreator
{
private:
	std::mutex lock;
	std::thread listener;
	std::unordered_map<SOCKET, std::thread> listeners;
	std::list<SOCKET> acceptSockets;
	std::unordered_map<SOCKET, std::atomic_bool> m_runningSockets;

	void ListenForSockets();
	void ListenForMessages(SOCKET& socket);
	void InitializeServerApplication();
protected:

public:
	ServerSideApplication();
	void Wait() noexcept;
};

