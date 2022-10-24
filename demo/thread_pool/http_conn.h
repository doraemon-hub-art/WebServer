 #ifndef HTTPCONNECTTION_H
 #define HTTPCONNECTTION_H

#include "common.h"
#include "locker.h"


// 连接对象类
class http_conn{

public:
    static int m_epollfd; // 所有socket上的事件都被注册到同一个epoll对象中
    static int m_user_count; // 统计用户数量
    static const int READ_BUFFER_SIZE=2048;  // 读缓冲区的大小
    static const int WRITE_BUFFER_SIZE=1024;// 写缓冲区的大小

    // HTTP请求方法，目前只支持GET
    enum METHOD{
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT
    };

    // 主状态机状态
    enum CHECK_STATE { 
        CHECK_STATE_REQUESTLINE = 0,  // 分析请求行
        CHECK_STATE_HEADER,           // 分析头部字段
        CHECK_STATE_CONTENT           // 当前正在解析请求行
    };

    // 从状态机状态-行的读取状态
    enum LINE_STATUS { 
        LINE_OK = 0,      // 读取到一个完整的行
        LINE_BAD,         // 行出错
        LINE_OPEN         // 行的数据尚不完整，就是还没到/r/n的/n
    };


    // 服务器处理http请求的结果，
    enum HTTP_CODE {
        NO_REQUEST,       // 请求不完整，需要继续读取客户数据
        GET_REQUEST,      // 获得了一个完整的客户请求
        BAD_REQUEST,      // 客户请求有语法错误
        FORBIDDEN_REQUEST,// 客户对资源没有足够的权限访问
        INTERNAL_ERROR,   // 服务器内部错误
        CLOSED_CONNECTION // 客户端已经关闭连接
    };


public:

    http_conn(){}
    ~http_conn(){}

    void process();// 处理客户端请求
    void init(int sockfd,const sockaddr_in & addr);// 初始化新接收的连接
    void close_conn();// 关闭连接
    bool read();//非阻塞读
    bool write();//非阻塞写
    HTTP_CODE process_read();                   // 解析HTTP请求
    HTTP_CODE parse_request_line(char*text);    //解析请求首行
    HTTP_CODE parse_headers(char* text);        //解析请求头
    HTTP_CODE parse_content(char* text);        // 解析请求内容
    
    HTTP_CODE do_request();                     // 具体的处理
    LINE_STATUS parse_line();                   //解析行

    void init();// 初始化其他信息
    char* get_line(){
        return m_read_buf + m_start_line;
    }


private:
    int m_sockfd;   // 该HTTP连接的socket
    sockaddr_in m_address;// 通信的socket地址
    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx;// 标识读缓冲区已经读入的客户端数据的最后一个字节的下一个位置

    int m_checked_index;// 当前正在分析的字符在读缓冲区中的位置
    int m_start_line;// 当前正在解析的行的起始位置
    CHECK_STATE m_check_state; // 主状态机当前所处的状态

    // 解析到的信息
    char* m_url; // 请求目标文件路径
    char* m_version; // 请求协议版本，http1.1
    METHOD m_method;// 请求方法
    char* m_host; // 主机名
    bool m_linger; // HTTP请求是否要保持连接
    int m_content_length;   // 消息体信息
};


 #endif