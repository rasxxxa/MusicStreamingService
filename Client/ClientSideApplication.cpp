#include "ClientSideApplication.h"

ClientSideApplication::ClientSideApplication() : SocketCreator(false)
{

}



void ClientSideApplication::ListenForMessage()
{
    std::shared_ptr<Sound> sound = std::make_shared<Sound>(false);
    bool firstTimePlay = false;
    unsigned long long memoryUsed = 0;
    std::vector<char> dataToPlay;
    while (true)
    {
       
        int result = Receive(mainSocket, 18, [&](void* object, int size)
            {
                std::vector<char> data, format;
                data = std::vector<char>((char*)object, (char*)object + size - 18);
                format = std::vector<char>((char*)object + size - 18, (char*)object + size);
                auto newFormat = Sound::ConvertDataToFormat(format);
                if (!firstTimePlay)
                {
                    dataToPlay.insert(dataToPlay.end(), data.begin(), data.end());
                    if (dataToPlay.size() >= newFormat.nAvgBytesPerSec * 2)
                    {
                        sound->PlayWithRowData(dataToPlay.data(), dataToPlay.size(), newFormat);
                        sound->PlaySource();
                        firstTimePlay = true;
                        memoryUsed += dataToPlay.size();
                        dataToPlay.clear();
                    }
                }
                else
                {
                    dataToPlay.insert(dataToPlay.end(), data.begin(), data.end());
                    if (dataToPlay.size() >= newFormat.nAvgBytesPerSec / 4)
                    {
                        sound->PlayWithRowData(dataToPlay.data(), dataToPlay.size(), newFormat);
                        memoryUsed += dataToPlay.size();
                        std::cout << memoryUsed << std::endl;
                        dataToPlay.clear();
                    }
                }
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
