#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <assert.h>
#include <mutex>
#include "connection.h"
using std::cout;
using std::endl;
using std::cerr;
using std::mutex;
using std::vector;
using std::stringstream;

mutex jsonMutex;
int clientSocketsLock = 0;
mutex clientSocketsLockMutex;

#define BATCH 100000+10

#pragma comment(lib, "Ws2_32.lib")

/*********************************************************************/
//Server
/*********************************************************************/
int Server::InitSocket()
{
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		LOG("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	ServerSocket = INVALID_SOCKET;
	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	u_long iMode = 1;
	ioctlsocket(ServerSocket, FIONBIO, &iMode);

	if (ServerSocket == INVALID_SOCKET) {
		LOG("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	LOG("init success\n");

}

void Server::SetAddr(string ip, string port)
{
	this->ipAddr = ip;
	this->port = port;

	InitSocket();
	Bind();

	/*recieveThread = thread(Start, this);
	recieveThread.detach();*/

}

void Server::Bind()
{
	LOG("binding...\n");

	int iResult;

	addrinfo hints = {}, *server_info = nullptr;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	getaddrinfo(ipAddr.c_str(), port.c_str(), &hints, &server_info);

	iResult = bind(ServerSocket, server_info->ai_addr, static_cast<int>(server_info->ai_addrlen));
	if (iResult == SOCKET_ERROR) {
		LOG("bind failed with error: %d\n", WSAGetLastError());

		closesocket(ServerSocket);
		freeaddrinfo(server_info);
		WSACleanup();
		return;
	}

	freeaddrinfo(server_info);

	if (listen(ServerSocket, SOMAXCONN) == SOCKET_ERROR) {

		LOG("Listen failed with error: %ld\n", WSAGetLastError());

		closesocket(ServerSocket);
		WSACleanup();
		return;
	}

	LOG("bind success\n");

}

int Server::Send(string buf)
{
	/*clientSocketsLockMutex.lock();
	LOG("lock...\n");
	clientSocketsLock = 1;
	clientSocketsLockMutex.unlock();*/
	Accept();

	for (vector<SOCKET>::iterator it = clientSockets.begin(); it != clientSockets.end(); )//iterate clients and send
	{
		LOG("sending...\n");
		//printf("sending json....");
		int iResult = send(*it, &buf[0], (int)(buf.size() * sizeof(char)), 0);
		//printf("\njson sent\n");
		if (iResult == SOCKET_ERROR)
		{//if error, close socket and delete
			LOG("send failed: %d\n", WSAGetLastError());
			//system("pause");
			closesocket(*it);
			it = clientSockets.erase(it);
			//printf("client deleted!\n");
			//printf("current client num: %d\n", clientSockets.size());

			clientNum--;
		}
		else
		{
			++it;
			LOG("send success!\n");
		}
	}

	/*clientSocketsLockMutex.lock();
	clientSocketsLock = 0;
	LOG("unlock...\n");
	clientSocketsLockMutex.unlock();*/

	return 0;
}

void Server::Receive()
{
	int iResult;
	stringstream ss;
	ss.str("");

	//receive buffer
	do {
		for (int j = 0; j < clientSockets.size(); ++j)
		{
			char buf[BATCH];
			//LOG("receiving\n");
			iResult = recv(clientSockets[j], buf, 100000, 0);
			if (!jsonQueue.empty())
			{
				string s = jsonQueue.front();
				//cout << s << endl;
				jsonQueue.pop();
			}
			//LOG("json size: %d\n", jsonQueue.size());

			if (iResult > 0) {
				//json segmentation
				LOG("recv success\n");
				for (int i = 0; i < iResult; ++i)
				{
					ss << buf[i];
					switch (buf[i])
					{
					case '{':
						charStack.push(buf[i]);
						break;

					case '}':
						assert('{' == charStack.top());
						charStack.pop();
						if (charStack.empty())
						{
							jsonMutex.lock();
							jsonQueue.push(ss.str());
							jsonMutex.unlock();
							ss.str("");
						}
						break;

					case '[':
						charStack.push(buf[i]);
						break;

					case ']':
						assert('[' == charStack.top());
						charStack.pop();
						if (charStack.empty())
						{
							jsonMutex.lock();
							jsonQueue.push(ss.str());
							jsonMutex.unlock();
							ss.str("");
						}
						break;
					}
				}
			}
			else if (iResult == 0)
			{
				LOG("Connection closing...\n");
			}
			else {
				LOG("recv failed: %d\n", WSAGetLastError());
				closesocket(clientSockets[j]);
				WSACleanup();
				mutex m;
				m.lock();
				clientNum--;
				m.unlock();
				return;
			}
		}
	} while (true);

}

void Server::Accept()
{
	//while (true)//repeatedly accept
	//{
	LOG("accepting...\n");
	SOCKET clientSocket = accept(ServerSocket, NULL, NULL);
	if (clientSocket == INVALID_SOCKET)
	{
		LOG("accept failed: %d\n", WSAGetLastError());
		closesocket(clientSocket);
	}
	else
	{
		clientSockets.push_back(clientSocket);
		clientNum++;
		cout << "new client connected!" << endl << "current client num: " << clientNum << endl;
		LOG("accepted!\n");
	}
	//}
}

void Server::Close()
{
	closesocket(ServerSocket);
	WSACleanup();
}
/*********************************************************************/
//Client
/*********************************************************************/
int Client::InitSocket()
{
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		LOG("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	ClientSocket = INVALID_SOCKET;
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ClientSocket == INVALID_SOCKET) {
		LOG("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	if (ClientSocket == INVALID_SOCKET) {
		LOG("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}
}

void Start(Client* client);

int Client::Connect(string ip, string port)
{
	InitSocket();

	this->ip = ip;
	this->port = port;

	recieveThread = thread(Start, this);
	recieveThread.detach();

	return 0;
}

void Client::ClientConnect()
{
	int iResult;

	addrinfo hints = {}, *server_info = nullptr;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	getaddrinfo(ip.c_str(), port.c_str(), &hints, &server_info);

	iResult = SOCKET_ERROR;
	while (iResult == SOCKET_ERROR)
	{
		LOG("connecting...\n");
		iResult = connect(ClientSocket, server_info->ai_addr, static_cast<int>(server_info->ai_addrlen));
		LOG("iresult %d\n", iResult);
	}

	LOG("connected!\n");
}

int Client::Send(string buf)
{
	printf("sending..\n");
	int iResult = send(ClientSocket, &buf[0], (int)(buf.size() * sizeof(char)), 0);
	if (iResult == SOCKET_ERROR) {
		LOG("send failed: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	return 0;
}

void Start(Client* client)
{
	client->ClientConnect();
	while (true)
	{
		client->Receive();
	}
}

void Client::Receive()
{
	int iResult;
	stringstream ss;
	ss.str("");

	//receive buffer
	do {
		char buf[BATCH];
		LOG("receiving\n");
		iResult = recv(ClientSocket, buf, 100000, 0);
		LOG("iResult %d\n", iResult);

		if (iResult > 0) {
			//json segmentation
			LOG("recv success\n");
			for (int i = 0; i < iResult; ++i)
			{
				ss << buf[i];
				switch (buf[i])
				{
				case '{':
					charStack.push(buf[i]);
					break;

				case '}':
					assert('{' == charStack.top());
					charStack.pop();
					if (charStack.empty())
					{
						jsonMutex.lock();
						clientJson = ss.str();
						jsonMutex.unlock();
						ss.str("");
					}
					break;

				case '[':
					charStack.push(buf[i]);
					break;

				case ']':
					assert('[' == charStack.top());
					charStack.pop();
					if (charStack.empty())
					{
						jsonMutex.lock();
						clientJson = ss.str();
						jsonMutex.unlock();
						ss.str("");
					}
					break;
				}
			}
		}
		else if (iResult == 0)
			LOG("Connection closing...\n");
		else {
			LOG("recv failed: %d\n", WSAGetLastError());
			/*closesocket(ClientSocket);
			WSACleanup();*/
			return;
		}

	} while (iResult > 0);

}

void Client::Close()
{
	closesocket(ClientSocket);
	ClientSocket = INVALID_SOCKET;
	WSACleanup();
}