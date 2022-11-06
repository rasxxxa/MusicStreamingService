#include "ServerSideApplication.h"

void ServerSideApplication::ListenForSockets()
{
	while (true)
	{
		if (listen(mainSocket, 1) == SOCKET_ERROR)
		{
			std::cout << "LISTEN ERROR" << std::endl;
			return;
		}
		else
		{
			std::cout << "LISTENING" << std::endl;
		}

		{
			std::lock_guard<std::mutex> guardLock(lock);
			SOCKET acceptSocket = accept(mainSocket, nullptr, nullptr);
			if (acceptSocket == INVALID_SOCKET)
			{
				std::cout << "ACCEPT FAILED" << WSAGetLastError() << std::endl;
				return;
			}

			acceptSockets.push_back(acceptSocket);
			listeners.emplace_back(&ServerSideApplication::ListenForMessages,this, std::ref(acceptSockets[acceptSockets.size() - 1]));
		}
	}
}

void ServerSideApplication::ListenForMessages(SOCKET& socket)
{
	while (true)
	{
		int result = Receive(socket, [](void* object, int size) {
			
			char* message = (char*)object;
			if (message != nullptr)
				std::cout << std::string(message, size);
			
			});
	}
}

void ServerSideApplication::InitializeServerApplication()
{
	listener = std::thread(&ServerSideApplication::ListenForSockets, this);
}

ServerSideApplication::ServerSideApplication() : SocketCreator(true)
{
	InitializeServerApplication();
}



void ServerSideApplication::Wait() noexcept
{
	while (true)
	{
		std::string toSend;
		std::cin >> toSend;
		for (auto& socket : acceptSockets)
		  Send(socket, toSend.c_str(), toSend.size());
	}
	listener.join();
}
