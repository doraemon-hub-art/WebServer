#include "http_conn.h"

// 向epoll中添加需要监听的文件描述符
void addfd(int epollfd,int fd,bool oneShot){
    epoll_event event;
    event.data.fd = fd;
    // 这里采用水平触发模式
    // 可以用EPOLLRDHUP事件来判断是否关闭
    event.events = EPOLLIN | EPOLLRDHUP;

    if(oneShot){// 保证一个socket在任一时刻只被一个线程处理
        event.events |= EPOLLONESHOT;
    }
    // 将事件添加到事件注册表中
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
}

// 从epoll中移除监听的文件描述符
void removefd(int epollfd,int fd){
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
}

// 修改epoll中监听事件的属性
void modfd(int epollfd,int fd,int ev){
    epoll_event event;
    event.data.fd = fd;
    // 重新给events属性赋值
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,0);
}


void http_conn::init(int sockfd,const sockaddr_in & addr){
    m_sockfd = sockfd;
    m_address = addr;

    // 端口复用
    int reuse = 1;
    setsockopt(m_sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));

    // 添加到epoll对象的内核事件表中
    addfd(m_epollfd,sockfd,true);
    m_user_count++;// 更新总用户数
    
}