 #ifndef HTTPCONNECTTION_H
 #define HTTPCONNECTTION_H

#include "common.h"
#include "locker.h"


// 连接对象类
class http_conn{

public:



    http_conn(){}
    ~http_conn();

    void process();// 处理客户端请求
    void init(int sockfd,const sockaddr_in & addr);// 初始化新接收的连接

public:
    static int m_epollfd; // 所有socket上的事件都被注册到同一个epoll对象中
    static int m_user_count; // 统计用户数量
private:
    int m_sockfd;   // 该HTTP连接的socket
    sockaddr_in m_address;// 通信的socket地址

};




 #endif