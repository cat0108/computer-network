#include"client.h"

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

void init_msg_header(UDP_HEADER& header,uint16_t src_port,uint16_t dst_port,uint16_t length,uint8_t seq,uint8_t flag)
{
    header.src_port=src_port;
    header.dst_port=dst_port;
    header.length=length;
    header.checksum=0;
    header.seq=seq;
    header.flag=flag;
}

int UDP_Client_connect(SOCKET& clientSocket,SOCKADDR_IN& serverAddr,int& serverAddrLen)
{
    cout<<"Client: "<<"begin to connect..."<<endl;
    char* recvbuffer=(char*)malloc(MAXBUFSIZE);
    char* sendbuffer=(char*)malloc(MAXBUFSIZE);
    UDP_HEADER header;
    //设置超时时间
    DWORD timeout=500;
    //设置超时选项
    if(setsockopt(clientSocket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout))==SOCKET_ERROR)
    {
        cerr<<"setsockopt error"<<endl;
        return -1;
    }

    //第一次握手，发送SYN报文
    ssize_t recvSize;
connect_first_step:
    memset(sendbuffer,0,MAXBUFSIZE);
    memset(recvbuffer,0,MAXBUFSIZE);
    init_header(header,CLIENT_PORT,SERVER_PORT,sizeof(header),0,0,SYN);
    memcpy(sendbuffer,&header,sizeof(header));
    if(sendto(clientSocket,sendbuffer,sizeof(header),0,(sockaddr*)&serverAddr,serverAddrLen)<=0)
    {
        cerr<<"send SYN segment error"<<endl;
        return -1;
    }
    cout<<"send SYN segment success"<<endl;
    //第二次握手，接收SYN_ACK报文
    //开始计时：
    if((recvSize=recvfrom(clientSocket,recvbuffer,MAXBUFSIZE,0,(sockaddr*)&serverAddr,&serverAddrLen))==SOCKET_ERROR)
    {
        //TODO:超时则自身重传SYN报文
        if(WSAGetLastError()==WSAETIMEDOUT)
        {
            cerr<<"time out,now is retransmission"<<endl;
            goto connect_first_step;
        }
    }
    memcpy(&header,recvbuffer,sizeof(header));
    if(UDP_checksum((uint16_t*)recvbuffer,recvSize)!=0)
    {
        cerr<<"SYN_ACK checksum error, now is retransmission"<<endl;
        goto connect_first_step;
    }
    if(header.flag!=SYN_ACK)
    {
        cerr<<"SYN_ACK flag error!"<<endl;
        return -1;
    }
    cout<<"Receive SYN_ACK segment..."<<endl;
    //第三次握手，发送ACK报文
connect_second_step:
    memset(sendbuffer,0,MAXBUFSIZE);
    init_header(header,CLIENT_PORT,SERVER_PORT,sizeof(header),0,0,ACK);
    memcpy(sendbuffer,&header,sizeof(header));
    if(sendto(clientSocket,sendbuffer,sizeof(header),0,(sockaddr*)&serverAddr,serverAddrLen)<=0)
    {
        cerr<<"send ACK segment failed,now going to resend...:"<<WSAGetLastError()<<endl;
        goto connect_second_step;
    }
    cout<<"send ACK segment success"<<endl;
    //TODO:进入等待接收状态，在一定时间内看对方是否有重传的报文(这里假定这个报文一定代表着重传信号)，若有则重传ACK报文,否则连接成功
    //这一段超时时间要设置的长一点
    timeout=1000;
    //设置超时选项
    if(setsockopt(clientSocket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout))==SOCKET_ERROR)
    {
        cerr<<"setsockopt error"<<endl;
        return -1;
    }
    if((recvSize=recvfrom(clientSocket,recvbuffer,MAXBUFSIZE,0,(sockaddr*)&serverAddr,&serverAddrLen))!=SOCKET_ERROR)
    {
        cerr<<"recv SYN_ACK segment again,now going to resend..."<<endl;
        goto connect_second_step;
    }
    
    free(recvbuffer);
    free(sendbuffer);
    cout<<"UDP Client connect success"<<endl<<endl;
    return 0;
}

int UDP_Client_disconnect(SOCKET& clientSocket,SOCKADDR_IN& serverAddr,int& serverAddrLen)
{
    cout<<"Client: "<<"begin to disconnect..."<<endl;
    char* sendbuffer=(char*)malloc(MAXBUFSIZE);
    char* recvbuffer=(char*)malloc(MAXBUFSIZE);
    UDP_HEADER header;
    //发送FIN报文
disconnect_first_step:
    memset(recvbuffer,0,MAXBUFSIZE);
    memset(sendbuffer,0,MAXBUFSIZE);
    init_header(header,CLIENT_PORT,SERVER_PORT,sizeof(header),0,0,FIN);
    memcpy(sendbuffer,&header,sizeof(header));
    if(sendto(clientSocket,sendbuffer,sizeof(header),0,(sockaddr*)&serverAddr,serverAddrLen)<=0)
    {
        cerr<<"send FIN segment failed,now going to resend...:"<<WSAGetLastError()<<endl;
        goto disconnect_first_step;
    }
    cout<<"send FIN segment success"<<endl;
    //接收FIN报文
    ssize_t recvSize;
    DWORD timeout=500;
    //设置超时选项
    if(setsockopt(clientSocket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout))==SOCKET_ERROR)
    {
        cerr<<"setsockopt error"<<endl;
        return -1;
    }
    if((recvSize=recvfrom(clientSocket,recvbuffer,MAXBUFSIZE,0,(sockaddr*)&serverAddr,&serverAddrLen))==SOCKET_ERROR)
    {
        //TODO；等待一段时间没有收到重新发送FIN报文
        if(WSAGetLastError()==WSAETIMEDOUT)
        {
            cerr<<"time out,now is retransmission"<<endl;
            goto disconnect_first_step;
        }
    }
    memcpy(&header,recvbuffer,sizeof(header));
    //检验和，若不正确则重传FIN报文
    if(UDP_checksum((uint16_t*)recvbuffer,recvSize)!=0)
    {
        cerr<<"checksum error, now is retransmission"<<endl;
        goto disconnect_first_step;
    }
    //判断是否为FIN报文
    if(header.flag!=FIN)
    {
        cerr<<"FIN flag error!"<<endl;
        return -1;
    }
    cout<<"Receive FIN segment success"<<endl;
    cout<<"UDP Client disconnect success"<<endl;
    free(sendbuffer);
    free(recvbuffer);
    return 0;
}

int UDP_Sendmsg(SOCKET& clientSocket,SOCKADDR_IN& serverAddr,int& serverAddrLen,char* sendbuffer,char* recvbuffer,size_t sendSize)
{
    UDP_HEADER header;
    ssize_t recvSize;
    DWORD timeout=500;
    //发送数据报文,其中sendbuffer已经封装好,包含了UDP报文头部
send_first_step:
    if(sendto(clientSocket,sendbuffer,sendSize,0,(sockaddr*)&serverAddr,serverAddrLen)<=0)
    {
        cerr<<"send data segment failed,now going to resend...:"<<WSAGetLastError()<<endl;
        goto send_first_step;
    }
    cout<<"send data segment success..."<<endl;
    //接收ACK报文
    if(setsockopt(clientSocket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout))==SOCKET_ERROR)
    {
        cerr<<"setsockopt error"<<endl;
        return -1;
    }
    if((recvSize=recvfrom(clientSocket,recvbuffer,MAXBUFSIZE,0,(sockaddr*)&serverAddr,&serverAddrLen))==SOCKET_ERROR)
    {
        //TODO:超时未收到重发数据报文
        if(WSAGetLastError()==WSAETIMEDOUT)
        {
            cerr<<"time out,now is retransmission"<<endl;
            goto send_first_step;
        }
    }
    memcpy(&header,recvbuffer,sizeof(header));
    //检验和，若不正确则重传数据报文
    if(UDP_checksum((uint16_t*)recvbuffer,recvSize)!=0)
    {
        cerr<<"recvmessage ACK checksum error, now is retransmission"<<endl;
        goto send_first_step;
    }
    //判断是否为ACK报文
    if(header.flag!=ACK)
    {
        cerr<<"ACK flag error!"<<endl;
        return -1;
    }
    //查看序列号是否和发送序列号一致
    if(header.seq!=seqnum)
    {
        cerr<<"seqnum error,now is retransmission"<<endl;
        goto send_first_step;
    }
    cout<<"sendmessage  success,message segnum is: "<<int(seqnum)<<endl<<"msgsize= "<<sendSize<<endl<<endl;
    return 0;
}

int main()
{
    WSADATA wasdata;
    if(WSAStartup(MAKEWORD(2,2),&wasdata)!=0)
    {
        cerr<<"WSAStartup error"<<endl;
        return -1;
    }
    SOCKET clientSocket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(clientSocket==INVALID_SOCKET)
    {
        cerr<<"socket error"<<endl;
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    }
    //设置客户端的端口并绑定
    SOCKADDR_IN clientAddr;
    clientAddr.sin_family=AF_INET;
    clientAddr.sin_port=htons(CLIENT_PORT);
    clientAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    if(bind(clientSocket,(sockaddr*)&clientAddr,sizeof(clientAddr))!=0)
    {
        cerr<<"bind error"<<endl;
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    }
    //设置服务器
    SOCKADDR_IN serverAddr;
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_port=htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    //三次握手
    int serverAddrLen=sizeof(serverAddr);
    if(UDP_Client_connect(clientSocket,serverAddr,serverAddrLen)!=0)
    {
        cerr<<"UDP client connect failed"<<endl;
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    }

    //开始发送数据
    char* sendbuffer=(char*)malloc(MAXBUFSIZE);
    char* recvbuffer=(char*)malloc(MAXBUFSIZE);
    memset(sendbuffer,0,MAXBUFSIZE);
    memset(recvbuffer,0,MAXBUFSIZE);
    UDP_HEADER header;

    //打开文件
    ifstream file("../testfile/1.jpg",ios::in|ios::binary);
    if(!file.is_open())
    {
        std::cerr << "Error opening file" << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    //读文件
    size_t readBytes=0;
    long totalBytes=0;
    //开始计时
    auto start=chrono::steady_clock::now();
    while(!file.eof())
    {
        init_msg_header(header,CLIENT_PORT,SERVER_PORT,0,seqnum,ACK);
        file.read(sendbuffer+sizeof(header),MAXBUFSIZE-sizeof(header));
        //获取实际读取到的字节
        readBytes=file.gcount();
        //填充length字段
        header.length=sizeof(header)+readBytes;
        memcpy(sendbuffer,&header,sizeof(header));
        //计算校验和
        header.checksum=UDP_checksum((uint16_t*)sendbuffer,header.length);
        memcpy(sendbuffer,&header,sizeof(header));
        //发送数据报文
        if(UDP_Sendmsg(clientSocket,serverAddr,serverAddrLen,sendbuffer,recvbuffer,header.length)!=0)
        {
            cerr<<"UDP sendmsg failed"<<endl;
            closesocket(clientSocket);
            WSACleanup();
            return -1;
        }
        seqnum++;
        totalBytes+=readBytes;
    }
    //结束计时
    auto end=chrono::steady_clock::now();
    auto duration=chrono::duration_cast<chrono::milliseconds>(end-start);
    cout<<"UDP sendmsg success,send "<<int(seqnum)<<" segments,use "<<duration.count()<<" ms"<<endl;
    cout<<"UDP send "<<totalBytes<<" bytes"<<endl;
    cout<<"throughput= "<<(totalBytes*8.0)/(duration.count())<<" kbps"<<endl;
    file.close();

    //结束连接
    memset(sendbuffer,0,MAXBUFSIZE);
    memset(recvbuffer,0,MAXBUFSIZE);
    init_header(header,CLIENT_PORT,SERVER_PORT,sizeof(header),0,0,FIN);
    memcpy(sendbuffer,&header,sizeof(header));
    if(UDP_Client_disconnect(clientSocket,serverAddr,serverAddrLen)!=0)
    {
        cerr<<"UDP client disconnect failed"<<endl;
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    }
    closesocket(clientSocket);
    WSACleanup();
    cout<<"UDP client is closed"<<endl;
    free(sendbuffer);
    free(recvbuffer);
    return 0;
}