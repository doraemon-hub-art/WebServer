#include "http_conn.h"

// 定义HTTP响应的一些状态信息
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";

// 网站的根目录
const char* doc_root = "/home/xuanxuan/coding/WebServer/resources";


// 静态成员初始化
int http_conn::m_epollfd = -1;
int http_conn::m_user_count = 0;

// 设置文件描述符非阻塞
void setnonblocking(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
}

// 向epoll中添加需要监听的文件描述符
void addfd(int epollfd,int fd,bool oneShot){
    epoll_event event;
    event.data.fd = fd;
    // 这里采用水平触发模式
    // 可以用EPOLLRDHUP事件来判断是否关闭
    //event.events = EPOLLIN | EPOLLRDHUP;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;


    if(oneShot){// 保证一个socket在任一时刻只被一个线程处理
        event.events |= EPOLLONESHOT;
    }
    // 将事件添加到事件注册表中
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    // 设置文件描述符非阻塞
    setnonblocking(fd);// 不影响水平触发
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

// 初始化连接对象
void http_conn::init(int sockfd,const sockaddr_in & addr){
    m_sockfd = sockfd;
    m_address = addr;

    // 端口复用
    int reuse = 1;
    setsockopt(m_sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));

    // 添加到epoll对象的内核事件表中
    addfd(m_epollfd,sockfd,true);
    m_user_count++;// 更新总用户数
    init();

}

// 关闭连接
void http_conn::close_conn(){
    if(m_sockfd != -1){
        removefd(m_epollfd,m_sockfd);
        m_sockfd = -1;
        m_user_count--;// 用户总数--
    }
}

// 非阻塞读-直到对方关闭连接
bool http_conn::read(){
    if(m_read_idx >= READ_BUFFER_SIZE){
        return false;
    }
    // 读取到的字节
    int byte_read = 0;
    while(true){
        byte_read = recv(m_sockfd,m_read_buf+m_read_idx,READ_BUFFER_SIZE-m_read_idx,0);
        if(byte_read == -1){
            if(errno == EAGAIN || errno == EWOULDBLOCK){// 没有数据啦
                break;
            }
            return false;
        }else if(byte_read == 0){// 对方关闭连接
            return false;
        }
        m_read_idx += byte_read;
    }
    printf("读取到了数据:%s\n",m_read_buf);
    return true;
}

// 非阻塞写
bool http_conn::write(){

    printf("读\n");
    return true;
}

void http_conn::init(){
    m_check_state = CHECK_STATE_REQUESTLINE; // 初始化状态为解析请求首行
    m_checked_index = 0;
    m_start_line = 0;
    m_read_idx = 0;

    m_method = GET;
    m_url = 0;
    m_version = 0;
    bzero(m_read_buf,READ_BUFFER_SIZE);
    m_linger = false;
    m_host = 0;
    m_content_length = 0;
}

// 主状态机，解析请求
http_conn::HTTP_CODE http_conn::process_read(){
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;

    char* text = 0;
    while( ((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK)) ||
        (line_status = parse_line()) == LINE_OK){
            // 解析到了一行完整的数据，或者解析到了请求体，也是完整的数据

            // 获取一行数据
            text = get_line();
            m_start_line = m_checked_index;
            printf("get 1 http line: %s\n",text);

            switch (m_check_state)
            {
                case CHECK_STATE_REQUESTLINE:{
                    ret = parse_request_line(text);
                    if(ret == BAD_REQUEST){// 不完整
                        return BAD_REQUEST;
                    }
                    break;
                }
                case CHECK_STATE_HEADER:{
                    ret = parse_headers(text);
                    if(ret == BAD_REQUEST){
                        return BAD_REQUEST;
                    }else if(ret == GET_REQUEST){
                        return do_request();
                    }
                    break;
                }
                case CHECK_STATE_CONTENT:{
                    ret = parse_content(text);
                    if(ret == GET_REQUEST){
                        return do_request();
                    }
                    line_status = LINE_OPEN;
                    break;
                }
                default:
                    return INTERNAL_ERROR;
            }
        return NO_REQUEST;
    }
}

// 解析http请求行，获取请求方法，目标URL，HTTP版本
http_conn::HTTP_CODE http_conn::parse_request_line(char*text){
    // GET / HTTP/1.1
    m_url = strpbrk(text," \t");

    *m_url++ = '\0';
    char* method = text;
    if(strcasecmp(method,"GET") == 0){
        m_method == GET;
    }else{
        return BAD_REQUEST;
    }

    m_version = strpbrk(m_url," \t");
    if(!m_version){
        return BAD_REQUEST;
    }
    *m_version++ = '\0';
    if(strcasecmp(m_version,"HTTP1.1") != 0){
        return BAD_REQUEST;
    }
    if(strncasecmp(m_url,"http://",7)){
        m_url += 7;
        m_url = strchr(m_url,'/');//找/第一次出现的索引
    }
    if(!m_url || m_url[0] != '/'){
        return BAD_REQUEST;
    }

    // 主状态机状态变化
    m_check_state = CHECK_STATE_HEADER;// 主状态机状态变成检查请求头
    return NO_REQUEST;
}
http_conn::HTTP_CODE http_conn::parse_headers(char* text){
    // 遇到空行，表示同步字段解析完毕。
    if(text[0] == '\0'){
        // 如果HTTP请求有消息体，则还需要读取m_content_length字节的消息体
        if(m_content_length != 0){
            m_check_state = CHECK_STATE_CONTENT;// 状态转义
            return NO_REQUEST;
        }
        // 否则我们已经得到一个完整的HTTP请求
        return GET_REQUEST;
    }else if(strncasecmp(text,"Connection:",11) == 0){
        // 处理Connection头部字段
        text += 11;
        text +=strspn(text," \t");
        if(strcasecmp(text,"keep-alive") == 0){
            m_linger = text;
        }
    }else if(strncasecmp(text,"Content-Length",15) == 0){
        // 处理Content-Length头部字段
        text += 15;
        text += strspn(text," \t");
        m_content_length = atol(text);
    }else if(strncasecmp(text,"Host:",5)==0){
        // 处理Host头部字段
        text += 5;
        text += strspn(text," \t");
        m_host = text;
    }else{
        printf("oop! unkown header: %s\n",text);
    }
    return NO_REQUEST;
}       

// 这里我们没有真正解析HTTP请求的消息体，只是判断它是否被完整的读入
http_conn::HTTP_CODE http_conn::parse_content(char* text){
    if(m_read_idx >= (m_content_length + m_checked_index)){
        text[m_content_length]='\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

// 解析一行数据，判断一行结尾/r/n
http_conn::LINE_STATUS http_conn::parse_line(){
    char temp;
    for(;m_checked_index < m_read_idx;m_checked_index++){
        temp = m_read_buf[m_checked_index];
        if(temp == '\r'){
            if((m_checked_index+1) == m_read_idx){
                return LINE_OPEN;// 不完整，没有\n
            }else if(m_read_buf[m_checked_index+1] == '\n'){
                m_read_buf[m_checked_index++] = '\0';// 字符串结尾设置结束符，成功拿到
                m_read_buf[m_checked_index++] = '\0';
                return LINE_OK;// 完整的解析到了一行
            }
            return LINE_BAD;
        }else if(temp == '\n'){// 考虑数据不是一次性发过来的
            // m_checked_index可以理解为防御性编程，小于1再-1就为负数了。
            if((m_checked_index > 1) && (m_read_buf[m_checked_index-1] == '\r')){
                m_read_buf[m_checked_index-1] = '\0';
                m_read_buf[m_checked_index++] ='\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        return LINE_OPEN;
    }
    return LINE_OK;
}                 

// 响应
http_conn::HTTP_CODE http_conn::do_request(){
    // /home/xuanxuan/coding/WebServer/resources
    // 真服了，tm这直接讲真不如带着敲，tm什么时候多出来这么多变量，jb倒是说一下啊，真服了。
    // 直接学习项目敲自己的了,md。
}

// 由线程池中的工作线程调用，这是处理HTTP请求的入口函数
void http_conn::process(){

    // 解析HTTP请求
    printf("parse request,create response\n");
    // 生成响应，即，把数据准备好

    HTTP_CODE read_ret =  process_read();
    if(read_ret == NO_REQUEST){
        modfd(m_epollfd,m_sockfd,EPOLLIN);
        return;
    }

}