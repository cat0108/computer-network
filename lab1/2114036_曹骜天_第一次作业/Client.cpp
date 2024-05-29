#include<WinSock2.h>
#include<iostream>
#include<string.h>
#include<string>
#include<WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;
#define BUFSIZE 1024
#define PORT 8000
//���շ�������Ϣ
DWORD WINAPI recvThread(LPVOID threadPara)
{
	SOCKET s = (SOCKET)threadPara;
	//ѭ�����շ�������Ϣ
	int ret = 0;
	do {
		char recvbuf[BUFSIZE] = { 0 };
		ret = recv(s, recvbuf, BUFSIZE, 0);
		cout << recvbuf << endl;
	} while (ret != SOCKET_ERROR && ret != 0);
	cout << "��������Ͽ�����" << endl;
	return 0;
}

int main()
{
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		cout << "WSAStartup error: " << WSAGetLastError() << endl;
		return 0;
	}
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		cout << "socket error: " << WSAGetLastError() << endl;
		return 0;
	}

	//���ӷ�������ip�Ͷ˿ں�
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);//�󶨱�����ַ
	if (connect(s, (SOCKADDR*)&addr, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		cout << "Connect Error: " << WSAGetLastError() << endl;
		return 0;
	}
	//�������߳̽��շ�������Ϣ
	HANDLE hThread = CreateThread(NULL, 0, recvThread, (LPVOID)s, 0, NULL);
	if (hThread == NULL)
	{
		cout << "create recvThread error: " << GetLastError() << endl;
	}
	CloseHandle(hThread);
	//������������Ϣ
	int ret = 0;
	int flag = 1;//�����˳�
	char buf[BUFSIZE] = { 0 };
	do {
		//��ȡһ��
		cin.getline(buf, BUFSIZE);
		std::cout << "\033[A\033[K";  // ����һ�в������ǰ��
		if (strcmp(buf, "exit")==0)
			flag = 0;
		ret = send(s, buf, BUFSIZE, 0);
		if (flag == 0)
			break;
	} while (ret != SOCKET_ERROR && ret != 0);
	closesocket(s);
	
	if (WSACleanup() != 0)
	{
		cout << "WSACleanup Error: " << WSAGetLastError() << endl;
	}
	return 0;
}
