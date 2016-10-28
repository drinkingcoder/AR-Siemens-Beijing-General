#pragma once
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
using std::string;

class Server
{
public:
	int InitSocket();
	int Bind(string ip, string port);
	int Accept();
	string Receive();
	void Close();

private:
	SOCKET ServerSocket;
};

class Client
{
public:
	int InitSocket();
	int Connect(string ip, string port);
	int Send(string str);
	void Close();

private:
	SOCKET ClientSocket;
};