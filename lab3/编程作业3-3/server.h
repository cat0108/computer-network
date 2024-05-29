#ifndef SERVER_H
#define SERVER_H

#include<iostream>
#include<WinSock2.h>
#include<stdint.h>
#include<fstream>
#include<ctime>
#include<chrono>
#include<vector>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define MAXBUFSIZE 4096
#define SERVER_PORT 8000
#define CLIENT_PORT 8002
#define WINDOW_SIZE 10


/*
    uint8_t：
    |0|0|0|SACK|END|FIN|ACK|SYN|
*/
const uint8_t SYN=0x01; 
const uint8_t ACK=0x02;
const uint8_t SYN_ACK=0x03;
const uint8_t FIN=0x04;
//用来标识传输完一个文件
const uint8_t END=0x08;
//用于标识报文的序列号,0-255.每次发送报文后加1
const uint8_t SACK=0x16;
uint8_t seqnum=0;
uint8_t last_seqnum=255;

ofstream file("../resultfile/1.jpg",ios::out|ios::binary);

//定义滑动窗
class Window
{
private:
    //窗口大小
    int size;
    //滑动窗的缓冲区
    vector<char*> buffer;
    //缓冲区中每个报文的大小
    vector<int> msg_size;
    //缓冲区中每个报文的序列号
    vector<uint8_t> msg_seq;
    //下一个添加的报文在缓冲区中的位置
    int next;
    //可以更新的滑动窗的状态数组，只有从初始位置到该位置的报文都已经确认，才可以更新
    vector<bool> update_status;
public:
    Window(): size(WINDOW_SIZE),next(0){};
    //添加报文到滑动窗缓冲区中
    int add_to_window(char* msg,int msg_size,uint8_t seq);
    //判断某个ack是否在滑动窗中,存在返回对应的下标，不存在返回-1
    int is_in_window(uint8_t ack);
    //判断滑动窗是否为空
    bool is_empty(){return next==0;};
    //返回next
    int get_next(){return next;};
    //返回对应的buffer
    char* get_buffer(int index){return buffer[index];};
    //返回对应的msg_size
    int get_msg_size(int index){return msg_size[index];};
    //返回对应的msg_seq
    uint8_t get_msg_seq(int index){return msg_seq[index];};
    //输出滑动窗的信息
    void print_window();
    //BR协议使用：接收到报文段后，将该ACK对应的报文从滑动窗中保存到文件里，前提是该报文之前的报文都已经被接收到
    int update_window_br();
};


Window* window=new Window();

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