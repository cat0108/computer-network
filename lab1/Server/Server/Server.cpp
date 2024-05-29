#include <winsock2.h> 
#include <iostream>
#include<WS2tcpip.h>
#include<string.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define BUFSIZE 1024
#define PORT 8000
#define MAXSOCK 10

SOCKET c[MAXSOCK];
int currentSize = 0;
int countLeft = 0;
//定义线程函数，与客户端进行通讯
DWORD WINAPI ThreadFun(LPVOID threadPara)
{
	//将LPVOID强制类型转换为SOCKET
	SOCKET client = (SOCKET)threadPara;

	cout << "now user: " << client << " is in the chatting room" <<endl<<"total user number = "<<(currentSize-countLeft)<< endl;

	//将信息写入buf中并发送
	char buf[BUFSIZE] = { 0 };
	sprintf_s(buf, "Server: now you: %d ,is in the chatting room,total user number: %d", client,currentSize-countLeft);
	send(client, buf, BUFSIZE, 0);
	for (int i = 0; i < currentSize; i++)
	{
		char broadcastBuf[BUFSIZE] = { 0 };
		if (c[i] != client)
		{
			sprintf_s(broadcastBuf, "Server: user %d is in the chatting room, total user number = %d", client, (currentSize - countLeft));
			send(c[i], broadcastBuf, BUFSIZE, 0);
		}
	}
	//接收客户端数据
	int ret = 0;//接收到的字节数
	do {
		char recvbuf[BUFSIZE] = { 0 };
		ret = recv(client, recvbuf, BUFSIZE, 0);
		if (strcmp(recvbuf, "exit") != 0)
		{
			for (int i = 0; i < currentSize; i++)
			{
				char broadcastBuf[BUFSIZE] = { 0 };
				sprintf_s(broadcastBuf, "%d say:", client);
				strcat_s(broadcastBuf, recvbuf);
				send(c[i], broadcastBuf, BUFSIZE, 0);
			}
			cout << client << ": " << recvbuf << endl;
		}
		else {
			cout <<"user: " << client << " is left--------------" << endl;
			for (int i = 0; i < currentSize; i++)
			{
				char broadcastBuf[BUFSIZE] = { 0 };
				sprintf_s(broadcastBuf, "Server: %d is exit !", client);
				strcat_s(broadcastBuf, recvbuf);
				send(c[i], broadcastBuf, BUFSIZE, 0);
			}
			countLeft++;
			closesocket(client);
			break;
		}
	} while (ret != SOCKET_ERROR && ret != 0);
	closesocket(client);
	return 0;
}

int main() {
	WSADATA wsadata;
	//初始化Winsock2
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		cout << "WSAStartup Error: " << WSAGetLastError << endl;
		return 0;
	}
	//创建套接字
	//地址类型：IPV4、套接字类型：流式套接字、协议类型：TCP
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		cout << "socket init error: " << WSAGetLastError() << endl;
		return 0;
	}
	//进行bind操作，绑定port和ip
	sockaddr_in addr;
	//先将字段全初始化为0
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;//IPV4
	addr.sin_port = htons(PORT);//将字节流转换网络字节流
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);//绑定本机地址
	if (bind(s, (SOCKADDR*)&addr, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		cout << "bind Error: " << WSAGetLastError() << endl;
		return 0;
	}

	//监听
	listen(s, 7);
	cout << "等待客户端连接------------" << endl;
	//与客户端连接
	while (1) {
		sockaddr_in Caddr;
		int len = sizeof(sockaddr_in);
		//创建新套接字和客户端通信
		c[currentSize] = accept(s, (SOCKADDR*)&Caddr, &len);
		//创建线程
		if (c[currentSize] != INVALID_SOCKET) {
			HANDLE hThread = CreateThread(NULL, 0, ThreadFun, (LPVOID)c[currentSize], 0, NULL);
			currentSize++;
			//关闭线程句柄、线程会自动执行到return
			if (hThread == NULL)
			{
				cout << "thread create Error!" << GetLastError() << endl;
				return 0;
			}
			else
				CloseHandle(hThread);
		}
	}
	closesocket(s);
	if (WSACleanup != 0)
	{
		cout << "WSACleanup Error: " << WSAGetLastError() << endl;
	}
	return 0;
}