#ifndef SERVER_H
#define SERVER_H

#include<iostream>
#include<WinSock2.h>
#include<stdint.h>
#include<fstream>
#include<ctime>
#include<chrono>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define MAXBUFSIZE 4096
#define SERVER_PORT 8000
#define CLIENT_PORT 8002


/*
    uint8_t：
    |0|0|0|0|0|FIN|ACK|SYN|
*/
const uint8_t SYN=0x01; 
const uint8_t ACK=0x02;
const uint8_t SYN_ACK=0x03;
const uint8_t FIN=0x04;
//用于标识报文的序列号,0-255.每次发送报文后加1
uint8_t seqnum=0;
//定义数据报文的结构
struct UDP_HEADER
{
    //16位源端口号与目的端口号
    uint16_t src_port;
    uint16_t dst_port;
    //16位UDP报文长度
    uint16_t length;
    //16位UDP检验和
    uint16_t checksum;
    //序列号
    uint8_t seq;
    //用于握手与挥手的标志位
    uint8_t flag;
};
//初始化UDP报文头部
void init_header(UDP_HEADER& header,uint16_t src_port,uint16_t dst_port,uint16_t length,uint16_t checksum,uint8_t seq,uint8_t flag);


/*
    * 用于计算UDP校验和
    * 参数：buffer为UDP数据报的首地址，size为UDP数据报的长度
    * 返回值：计算得到的校验和
*/
uint16_t UDP_checksum(uint16_t* buffer,int size);
//自定义三次握手
int UDP_Server_connect(SOCKET& serverSocket,SOCKADDR_IN& clientAddr,int& clientAddrLen);

int UDP_Recvmsg(SOCKET& serverSocket,SOCKADDR_IN& clientAddr,int& clientAddrLen,char* recvbuffer,char* sendbuffer);
#endif