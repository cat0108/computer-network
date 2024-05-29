#ifndef CLIENT_H

#include<iostream>
#include<WinSock2.h>
#include<stdint.h>
#include<fstream>
#include<windows.h>
#include<ctime>
#include<chrono>
#include<vector>
#include<algorithm>
#pragma comment(lib, "ws2_32.lib")

using namespace std;
//lab3.2，采用滑动窗机制，累计确认


#define MAXBUFSIZE 4096
#define SERVER_PORT 8002
#define CLIENT_PORT 8001
#define WINDOW_SIZE 10
#define TIMEOUT 500

/*
    uint8_t：
    |0|0|0|0|END|FIN|ACK|SYN|
*/
const uint8_t SYN=0x01; 
const uint8_t ACK=0x02;
const uint8_t SYN_ACK=0x03;
const uint8_t FIN=0x04;
const uint8_t END=0x08;
//用于标识报文的序列号,0-255.每次发送报文后加1

uint8_t seqnum=0;


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
public:
    Window(): size(WINDOW_SIZE),next(0){};
    //添加报文到滑动窗缓冲区中
    int add_to_window(char* msg,int msg_size,uint8_t seq);
    //接收到ACK后，将已经确认的报文从滑动窗中删除
    int update_window(uint8_t ack);
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
};


//定义计时器
class Timer
{
private:
    //计时器的超时时间
    int timeout;
    //计时器的开始时间
    chrono::steady_clock::time_point start;
    //计时器是否启动
    bool isStart;
    //计时器结束时的时间
    chrono::steady_clock::time_point end;
    //计时器为window里的第一个报文计时
    uint8_t seq;
public:
    Timer():timeout(TIMEOUT),isStart(false){};
    //启动计时器
    void start_timer(uint8_t seq);
    //停止计时并计算是否超时
    bool is_timeout();
    //关闭计时器
    void close_timer();
    //返回seq
    uint8_t get_seq(){return seq;};

};

Window* window=new Window();
Timer* timer=new Timer();

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

//接受消息的线程函数的参数
struct thread_param
{
    SOCKET clientSocket;
    SOCKADDR_IN serverAddr;
    int serverAddrLen;
    char* recvbuffer;
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
//客户端发送消息
int UDP_Sendmsg(SOCKET& clientSocket,SOCKADDR_IN& serverAddr,int& serverAddrLen,char* sendbuffer,size_t sendSize);
//新建线程用于接收ACK并更新滑动窗
DWORD WINAPI recv_thread(LPVOID lpParameter);

#define CLIENT_H
#endif