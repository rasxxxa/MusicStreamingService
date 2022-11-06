#include "ClientSideApplication.h"

ClientSideApplication::ClientSideApplication() : SocketCreator(false)
{

}



void ClientSideApplication::ListenForMessage()
{
    while (true)
    {
        int result = Receive(mainSocket, [](void* object, int size)
            {
                // Handle received
                char* message = (char*)object;
                if (message != nullptr)
                    std::cout << std::string(message, size);

            });
    }
}

void ClientSideApplication::Wait() noexcept
{
    listener = std::thread(&ClientSideApplication::ListenForMessage, this);
    while (true)
    {
        std::string message;
        std::cin >> message;
        Send(mainSocket, message.data(), message.size());
    }
}
