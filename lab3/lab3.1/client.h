#ifndef CLIENT_H

#include<iostream>
#include<WinSock2.h>
#include<stdint.h>
#include<fstream>
#include<ctime>
#include<chrono>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define MAXBUFSIZE 4096
#define SERVER_PORT 8002
#define CLIENT_PORT 8001


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
/*
 *初始化UDP报文头部
 *notice:此初始化函数会自动计算校验和，当且仅当报文段只包含头部
 *若发送的报文段包含数据，则需要在发送前自行计算校验和
*/
void init_header(UDP_HEADER& header,uint16_t src_port,uint16_t dst_port,uint16_t length,uint16_t checksum,uint8_t seq,uint8_t flag);

//此函数只将校验和部分初始化为0
void init_msg_header(UDP_HEADER& header,uint16_t src_port,uint16_t dst_port,uint16_t length,uint8_t seq,uint8_t flag);

//客户端的三次握手
int UDP_Client_connect(SOCKET& clientSocket,SOCKADDR_IN& serverAddr,int& serverAddrLen);
//客户端的两次挥手
int UDP_Client_disconnect(SOCKET& clientSocket,SOCKADDR_IN& serverAddr,int& serverAddrLen);
//客户端发送消息并接收ACK
int UDP_Sendmsg(SOCKET& clientSocket,SOCKADDR_IN& serverAddr,int& serverAddrLen,char* sendbuffer,char* recvbuffer,size_t sendSize);
#define CLIENT_H
#endif