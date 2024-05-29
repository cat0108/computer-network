#include "server.h"
#include<WS2tcpip.h>


uint16_t UDP_checksum(uint16_t* buffer,int size)
{
    //以16位为单位加法，先计算出总共需要加的次数
    int count = (size+1)/2;
    uint16_t* buf=(uint16_t*)malloc(size+1);
    memset(buf,0,size+1);
    memcpy(buf,buffer,size);
    //32位用来判断是否进位
    uint32_t sum=0;
    while (count--)
    {
        sum+=*buf++;
        //判断是否需要进位
        if(sum&0xffff0000)
        {
            sum&=0xffff;
            sum++;
        }
    }
    //返回的为16位
    return ~(sum&0xffff);
    
}

void init_header(UDP_HEADER& header,uint16_t src_port,uint16_t dst_port,uint16_t length,uint16_t checksum,uint8_t seq,uint8_t flag)
{
    header.src_port=src_port;
    header.dst_port=dst_port;
    header.length=length;
    header.checksum=checksum;
    header.seq=seq;
    header.flag=flag;
    header.checksum=UDP_checksum((uint16_t*)&header,sizeof(header));
}

int UDP_Server_connect(SOCKET& serverSocket,SOCKADDR_IN& clientAddr,int& clientAddrLen)
{
    cout<<"Server: "<<"begin to connect..."<<endl;
    char* recvbuffer=(char*)malloc(MAXBUFSIZE);
    char* sendbuffer=(char*)malloc(MAXBUFSIZE);
    memset(recvbuffer,0,MAXBUFSIZE);
    memset(sendbuffer,0,MAXBUFSIZE);
    UDP_HEADER header;
    DWORD timeout=500;
    //第一次握手，接收SYN报文
    ssize_t recvSize;
    //等待接收SYN报文
connect_first_step:
    //TODO:服务器等待建立连接,阻塞就行
    recvSize=recvfrom(serverSocket,recvbuffer,MAXBUFSIZE,0,(sockaddr*)&clientAddr,&clientAddrLen);
    if(recvSize==SOCKET_ERROR)
    {
        cerr<<"receive SYN segment failed,please resend...:"<<WSAGetLastError()<<endl;
        goto connect_first_step;
    }
    memcpy(&header,recvbuffer,sizeof(header));
    //检验和,若校验和不为0，则出错，接收重传信息
    if (UDP_checksum((uint16_t*)recvbuffer,recvSize)!=0)
    {
        cerr<<"SYN checksum error ,now is retransmission "<<endl;
        goto connect_first_step;
    }
    //判断是否为SYN报文
    if(header.flag!=SYN)
    {
        cerr<<"SYN flag error!"<<endl;
        return -1;
    }
    cout<<"Receive SYN Segment..."<<endl;
    //TODO：设置超时选项
    if(setsockopt(serverSocket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout))==SOCKET_ERROR)
    {
        cerr<<"setsockopt error"<<endl;
        return -1;
    }
    //第二次握手,发送SYN_ACK报文
    init_header(header,SERVER_PORT,CLIENT_PORT,sizeof(header),0,0,SYN_ACK);
    memcpy(sendbuffer,&header,sizeof(header));
connect_second_step:
    if(sendto(serverSocket,sendbuffer,sizeof(header),0,(sockaddr*)&clientAddr,clientAddrLen)<=0)
    {
        cerr<<"send SYN_ACK segment failed,now going to resend...:"<<WSAGetLastError()<<endl;
        goto connect_second_step;
    }
    cout<<"send SYN_ACK Segment..."<<endl;
    //第三次握手，接收ACK报文
    memset(recvbuffer,0,MAXBUFSIZE);
    if((recvSize=recvfrom(serverSocket,recvbuffer,MAXBUFSIZE,0,(sockaddr*)&clientAddr,&clientAddrLen))==SOCKET_ERROR)
    {
        //todo:如果超时，重传SYN_ACK报文
        if(WSAGetLastError()==WSAETIMEDOUT)
        {
            cerr<<"receive ACK segment timeout, now is retransmission"<<endl;
            goto connect_second_step;
        }

    }
    memcpy(&header,recvbuffer,sizeof(header));
    //检验和，若不正确则重传SYN_ACK报文,提醒客户端重传ACK报文
    if (UDP_checksum((uint16_t*)recvbuffer,recvSize)!=0)
    {
        cerr<<"ACK checksum error, now is retransmission"<<endl;
        goto connect_second_step;
    }
    //判断是否为ACK报文
    if(header.flag!=ACK)
    {
        cerr<<"ACK flag error, now is retransmission"<<endl;
        return -1;
    }
    cout<<"Receive ACK segment..."<<endl;
    cout<<"UDP connect success"<<endl;
    free(sendbuffer);
    free(recvbuffer);
    return 0;
}

int UDP_Recvmsg(SOCKET& serverSocket,SOCKADDR_IN& clientAddr,int& clientAddrLen,char* recvbuffer,char* sendbuffer)
{
    UDP_HEADER header;
    char* msg=(char*)malloc(MAXBUFSIZE-sizeof(header));
    memset(msg,0,MAXBUFSIZE-sizeof(header));
    ssize_t recvSize;
    DWORD timeout=500;
    uint8_t last_seqnum=255;
    ofstream file("../resultfile/1.jpg",ios::out|ios::binary);
    if(!file.is_open())
    {
        cerr<<"error:open recvfile failed!"<<endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    while(1)
    {
        //收到信息
        if((recvSize=recvfrom(serverSocket,recvbuffer,MAXBUFSIZE,0,(sockaddr*)&clientAddr,&clientAddrLen))>0)
        {
            memcpy(&header,recvbuffer,sizeof(header));
            //校验和
            if(UDP_checksum((uint16_t*)recvbuffer,recvSize)!=0)
            {
                cerr<<"ACK checksum error, now is retransmission"<<endl;
                continue;
            }
            //接收到普通消息
            if(header.flag==ACK)
            {
                cout<<"recieve msg success, msg seqnum= "<<int(header.seq)<<endl<<"msgsize= "<<recvSize<<endl;
                memcpy(msg,recvbuffer+sizeof(header),recvSize-sizeof(header));
                //TODO:将消息写入文件,比较seqnum和last_seqnum来确定是否是重传的消息
                if(recvSize-sizeof(header)>0  && header.seq!=last_seqnum)
                {
                    file.write(msg,recvSize-sizeof(header));
                    last_seqnum=header.seq;
                }
                if(header.seq==last_seqnum)
                {
                    cout<<"receive repeat seqnum, now is retransmission ACK"<<endl;
                }
                //发送ACK报文给客户端,发送相同的序列号
                init_header(header,SERVER_PORT,CLIENT_PORT,sizeof(header),0,header.seq,ACK);
                memcpy(sendbuffer,&header,sizeof(header));
                if(sendto(serverSocket,sendbuffer,sizeof(header),0,(sockaddr*)&clientAddr,clientAddrLen)<=0)
                {
                    cerr<<"send ACK segment failed,now going to resend...:"<<WSAGetLastError()<<endl;
                    continue;
                }
                cout<<"send ACK segment success, seqnum= "<<int(header.seq)<<endl<<endl;
                memset(msg,0,MAXBUFSIZE-sizeof(header));
            }
            //接收到FIN报文
            else if(header.flag==FIN)
            {
                cout<<"receive FIN segment success, begin to disconnect..."<<endl;
            disconnect:
                //得到消息关闭连接，同时发给客户端FIN报文
                init_header(header,SERVER_PORT,CLIENT_PORT,sizeof(header),0,0,FIN);
                memcpy(sendbuffer,&header,sizeof(header));
                if(sendto(serverSocket,sendbuffer,sizeof(header),0,(sockaddr*)&clientAddr,clientAddrLen)<=0)
                {
                    cerr<<"send FIN segment failed,now going to resend...:"<<WSAGetLastError()<<endl;
                    continue;
                }
                cout<<"send FIN segment success!"<<endl;
                //TODO:等待一段时间，对方没有发来消息
                timeout=1000;
                if(setsockopt(serverSocket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout))==SOCKET_ERROR)
                {
                    cerr<<"setsockopt error"<<endl;
                    return -1;
                }
                if((recvSize=recvfrom(serverSocket,recvbuffer,MAXBUFSIZE,0,(sockaddr*)&clientAddr,&clientAddrLen))!=SOCKET_ERROR)
                {
                    //有收到信息，默认为FIN，进行重传
                    cerr<<"receive FIN segment again, now is retransmission"<<endl;
                    goto disconnect;
                }
                //关闭连接
                cout<<"UDP server disconnect success"<<endl;
                cout<<"UDP server is closed"<<endl;
                break;
            }
        }
        memset(recvbuffer,0,MAXBUFSIZE);
        memset(sendbuffer,0,MAXBUFSIZE);
    }
    file.close();
    return 0;
}

int main()
{
    WSADATA wsadata;
    if(WSAStartup(MAKEWORD(2,2),&wsadata)!=0)
    {
        cerr<<"WSAStartup failed"<<endl;
        return -1;
    }

    SOCKET serverSocket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(serverSocket==INVALID_SOCKET)
    {
        cerr<<"socket failed"<<WSAGetLastError()<<endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }
    //设置服务器
    sockaddr_in serverAddr;
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    //inet_pton(AF_INET,"127.0.0.1",&serverAddr.sin_addr.s_addr);
    serverAddr.sin_port=htons(SERVER_PORT);
    if(bind(serverSocket,(sockaddr*)&serverAddr,sizeof(serverAddr))==SOCKET_ERROR)
    {
        cerr<<"bind failed"<<WSAGetLastError()<<endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }
    //绑定套接字，进入监听状态
    cout<<"UDP server is listening..."<<endl;

    sockaddr_in clientAddr;
    socklen_t clientAddrLen=sizeof(clientAddr);
    if(UDP_Server_connect(serverSocket,clientAddr,clientAddrLen)==-1)
    {
        cerr<<"UDP connect failed"<<endl;
        cerr<<"UDP server is closed"<<endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    cout<<"UDP server is connected"<<endl<<endl;
    //传输的主体部分，接收用户的消息
    char* recvbuffer=(char*)malloc(MAXBUFSIZE);
    char* sendbuffer=(char*)malloc(MAXBUFSIZE);
    memset(recvbuffer,0,MAXBUFSIZE);
    memset(sendbuffer,0,MAXBUFSIZE);

    UDP_Recvmsg(serverSocket,clientAddr,clientAddrLen,recvbuffer,sendbuffer);

    free(sendbuffer);
    free(recvbuffer);
    closesocket(serverSocket);
    WSACleanup();
}