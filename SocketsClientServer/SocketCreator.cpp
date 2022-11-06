#include "SocketCreator.h"

SocketCreator::SocketCreator(bool isServer) : mainSocket(0)
{
	WSADATA data;
	WORD versionRequested = MAKEWORD(2, 2);
	int error = WSAStartup(versionRequested, &data);
	if (error != 0)
	{
		std::cout << "Winsock dll not found" << std::endl;
		return;
	}
	CreateMainSocket();
	ConnectSocket(isServer);
}

int SocketCreator::Send(SOCKET socket, const void const* object, int objectSize)
{
	return send(socket, (char*)object, objectSize, 0);
}



int SocketCreator::Receive(SOCKET socket, const std::function<void(void*, int bytesReceived)>& handleObject)
{
	void* objectReceived = (void*)malloc(MAX_BUFFER_SIZE);
	int result = -1;
	if (objectReceived != nullptr)
	    result = recv(socket, (char*)objectReceived, MAX_BUFFER_SIZE, 0);
	
	if (result >= 0)
		handleObject(objectReceived, result);
	else
		CloseSocket();
	free(objectReceived);
	return result;
}

void SocketCreator::CloseSocket() const noexcept
{
	closesocket(mainSocket);
}



void SocketCreator::CreateMainSocket() noexcept
{
	mainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (mainSocket == INVALID_SOCKET)
	{
		std::cout << "INVALID SOCKET" << std::endl;
		DoCleanup();
	}
}



void SocketCreator::ConnectSocket(bool isServer) noexcept
{
	sockaddr_in service;
	service.sin_family = AF_INET;
	InetPton(AF_INET, _T("127.0.0.1"), &service.sin_addr.S_un.S_addr);
	service.sin_port = htons(PORT);
	auto func = (isServer)? &bind : &connect;
	if (func(mainSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR)
	{
		std::cout << "CONNECTION FAILED!" << WSAGetLastError() << std::endl;
		CloseSocket();
		DoCleanup();
	}
}

void SocketCreator::StartUpSocket() noexcept
{
}

void SocketCreator::DoCleanup() noexcept
{
	WSACleanup();
}