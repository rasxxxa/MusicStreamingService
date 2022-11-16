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
			listeners[acceptSocket] = std::thread(&ServerSideApplication::ListenForMessages, this, std::ref(acceptSockets.back()));
		}
	}
}

void ServerSideApplication::ListenForMessages(SOCKET& socket)
{
	while (true && m_runningSockets[socket])
	{
		int result = Receive(socket,0, [](void* object, int size) {
			
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
	std::vector<std::shared_ptr<Sound>> sounds;
	for (int i = 0; i < 4; i++)
	{
		sounds.push_back(std::make_shared<Sound>("../Music/Song" + std::to_string(i) + ".wav", false));
	}
	std::string s;
	std::cin >> s;

	for (short song = 0; song < sounds.size(); song++)
	{
		std::deque<std::vector<char>> elems;
		int sleepValue = 0;
		sounds[song]->GetDividedData(elems, 4096 , &sleepValue);
		auto format = sounds[song]->GetSoundFile().GetWaveFormat();

		auto res = Sound::ConvertMyWaveFormatToData(format);

		while (!elems.empty())
		{
			std::vector<char> data = elems.front();
			data.insert(data.end(), res.begin(), res.end());
			for (auto it = acceptSockets.begin(); it != acceptSockets.end();)
			{
				auto res = Send(*it, data.data(), data.size());
				if (res == -1)
				{
					m_runningSockets[*it] = false;
					it = acceptSockets.erase(it);
				}
				else
				{
					it++;
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(sleepValue-10));
			elems.pop_front();
			
		}
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(1000000));
	listener.join();
}
