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
//�����̺߳�������ͻ��˽���ͨѶ
DWORD WINAPI ThreadFun(LPVOID threadPara)
{
	//��LPVOIDǿ������ת��ΪSOCKET
	SOCKET client = (SOCKET)threadPara;

	cout << "now user: " << client << " is in the chatting room" <<endl<<"total user number = "<<(currentSize-countLeft)<< endl;

	//����Ϣд��buf�в�����
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
	//���տͻ�������
	int ret = 0;//���յ����ֽ���
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
	//��ʼ��Winsock2
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		cout << "WSAStartup Error: " << WSAGetLastError << endl;
		return 0;
	}
	//�����׽���
	//��ַ���ͣ�IPV4���׽������ͣ���ʽ�׽��֡�Э�����ͣ�TCP
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		cout << "socket init error: " << WSAGetLastError() << endl;
		return 0;
	}
	//����bind��������port��ip
	sockaddr_in addr;
	//�Ƚ��ֶ�ȫ��ʼ��Ϊ0
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;//IPV4
	addr.sin_port = htons(PORT);//���ֽ���ת�������ֽ���
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);//�󶨱�����ַ
	if (bind(s, (SOCKADDR*)&addr, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		cout << "bind Error: " << WSAGetLastError() << endl;
		return 0;
	}

	//����
	listen(s, 7);
	cout << "�ȴ��ͻ�������------------" << endl;
	//��ͻ�������
	while (1) {
		sockaddr_in Caddr;
		int len = sizeof(sockaddr_in);
		//�������׽��ֺͿͻ���ͨ��
		c[currentSize] = accept(s, (SOCKADDR*)&Caddr, &len);
		//�����߳�
		if (c[currentSize] != INVALID_SOCKET) {
			HANDLE hThread = CreateThread(NULL, 0, ThreadFun, (LPVOID)c[currentSize], 0, NULL);
			currentSize++;
			//�ر��߳̾�����̻߳��Զ�ִ�е�return
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