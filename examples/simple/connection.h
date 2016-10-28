#pragma once
#include <string>
#include <queue>
#include <stack>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
using std::string;
using std::thread;
using std::queue;
using std::stack;
using std::vector;

//#define LOG_OPEN
#ifdef LOG_OPEN
#define LOG(X) printf(X)
#else
#define LOG(X)
#endif
class Server
{
public:
	Server()
	{
		clientNum = 0;
	}

	void SetAddr(string ip, string port);
	void Close();
	int Send(string str);
	void Receive();

private:
	thread recieveThread;
	SOCKET ServerSocket;
	stack<char> charStack;
	void Bind();
	void Accept();
	int InitSocket();
	string ipAddr;
	string port;
	vector<SOCKET> clientSockets;
	int clientNum;

public:
	queue<string> jsonQueue;
};

class Client
{
public:
	int Connect(string ip, string port);
	int Send(string str);
	void Receive();
	void ClientConnect();
	void Close();

	stack<char> charStack;
	string clientJson;

private:
	thread recieveThread;
	int InitSocket();
	string ip, port;
	SOCKET ClientSocket;
};