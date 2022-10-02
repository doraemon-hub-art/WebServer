#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096 // 读缓冲区大小

// 主状态机的2种可能状态
enum CHECK_STATE { 
    CHECK_STATE_REQUESTLINE = 0,  // 分析请求行
    CHECK_STATE_HEADER,           // 分析头部字段
   //CHECK_STATE_CONTENT          
};

// 从状态机的3种可能状态——行的读取状态
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

// 为了简化问题，本代码没有给客户端发送一个完整的HTTP应答报文，而只是根据服务器的处理结果发送如下成功or失败信息。
static const char* szret[] = { 
    "I get a correct result\n", 
    "Something wrong\n" 
};


// 从状态机，用于解析出一行的内容。
LINE_STATUS parse_line( char* buffer, int& checked_index, int& read_index )
{
    // read_index指向buffer中当前分析数据的下一个字节
    // checked_index指向buffer中当前分析数据中正在分析的字节
    // 也就是说这里分析的是checked_index~read_index中的数据
    // 逐字节分析
    char temp;
    for ( ; checked_index < read_index; ++checked_index )
    {
        temp = buffer[ checked_index ];// 拿到当前要分析的字节(字符)
        if ( temp == '\r' ){// 当前为\r说明可能读取到一个完整的行
        /*
            如果\r是当前已经读到的数据的最后一个,说明当前还没有读取到一个完整行，
            需要继续读取客户端数据才能进一步分析。
        */
            if ( ( checked_index + 1 ) == read_index ){
                return LINE_OPEN;// 返回数据不完整
            }
            // 下一个就是\n,说明我们读取到了一个完整的行。
            else if ( buffer[ checked_index + 1 ] == '\n' ){
                buffer[ checked_index++ ] = '\0';// 添加字符串结束符
                buffer[ checked_index++ ] = '\0';
                // 读取到完整的一行，准备交给主状态机进行处理
                return LINE_OK;// 读取到完整的一行
            }
            return LINE_BAD;// 否则返回当前行数据出错
        }
        else if( temp == '\n' ){//当前的字符是\n,说明也可能读到一个完整的行
            // 进一步判断
            if( ( checked_index > 1 ) &&  buffer[ checked_index - 1 ] == '\r' ){
                buffer[ checked_index-1 ] = '\0';
                buffer[ checked_index++ ] = '\0';
                return LINE_OK; // 是完整行
            }
            return LINE_BAD;// 行出错
        }
    }
    return LINE_OPEN;// 行数据不完整
}

HTTP_CODE parse_requestline( char* szTemp, CHECK_STATE& checkstate )
{
    char* szURL = strpbrk( szTemp, " \t" );
    if ( ! szURL ){
        return BAD_REQUEST;
    }
    *szURL++ = '\0';

    char* szMethod = szTemp;
    if ( strcasecmp( szMethod, "GET" ) == 0 ){
        printf( "The request method is GET\n" );
    }
    else{
        return BAD_REQUEST;
    }

    szURL += strspn( szURL, " \t" );
    char* szVersion = strpbrk( szURL, " \t" );
    if ( ! szVersion ){
        return BAD_REQUEST;
    }
    *szVersion++ = '\0';
    szVersion += strspn( szVersion, " \t" );
    if ( strcasecmp( szVersion, "HTTP/1.1" ) != 0 ){
        return BAD_REQUEST;
    }

    if ( strncasecmp( szURL, "http://", 7 ) == 0 ){
        szURL += 7;
        szURL = strchr( szURL, '/' );
    }

    if ( ! szURL || szURL[ 0 ] != '/' ){
        return BAD_REQUEST;
    }

    //URLDecode( szURL );
    printf( "The request URL is: %s\n", szURL );
    checkstate = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

HTTP_CODE parse_headers( char* szTemp )
{
    if ( szTemp[ 0 ] == '\0' )
    {
        return GET_REQUEST;
    }
    else if ( strncasecmp( szTemp, "Host:", 5 ) == 0 )
    {
        szTemp += 5;
        szTemp += strspn( szTemp, " \t" );
        printf( "the request host is: %s\n", szTemp );
    }
    else
    {
        printf( "I can not handle this header\n" );
    }

    return NO_REQUEST;
}


// 分析http请求的入口函数-从这里开始进行处理
/*

        接收缓冲区
        当前已经分析完了多少字节的数据
        主状态机的初始状态
        当前已经读取了多少字节的数据
        接收缓冲区中的起始位置

*/ 
HTTP_CODE parse_content( char* buffer, int& checked_index, CHECK_STATE& checkstate, int& read_index, int& start_line ){

    LINE_STATUS linestatus = LINE_OK;
    HTTP_CODE retcode = NO_REQUEST;
    // 如果没读到头，就继续处理。
    while( ( linestatus = parse_line( buffer, checked_index, read_index ) ) == LINE_OK ){// 一行一行开始解析

        // 成功读全了一行， 进入到这里
        char* szTemp = buffer + start_line;// 当前读取到的一行数据头
        start_line = checked_index;// 更新下标，下一行的起始位置。
        switch ( checkstate ){// 主状态机的当前状态
            case CHECK_STATE_REQUESTLINE:{// 分析请求行
                retcode = parse_requestline( szTemp, checkstate );
                if ( retcode == BAD_REQUEST ){
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER:{// 分析头部字段
                retcode = parse_headers( szTemp );
                if ( retcode == BAD_REQUEST ){
                    return BAD_REQUEST;
                }
                else if ( retcode == GET_REQUEST )
                {
                    return GET_REQUEST;
                }
                break;
            }
            default:    // 报错
            {
                return INTERNAL_ERROR;
            }
        }
    }
    if( linestatus == LINE_OPEN )
    {
        return NO_REQUEST;
    }
    else
    {
        return BAD_REQUEST;
    }
}

int main( int argc, char* argv[] ){

    if( argc <= 2 ){
        printf( "usage: %s ip_address port_number\n", basename( argv[0] ) );
        return 1;
    }

    const char* ip = argv[1];   
    int port = atoi( argv[2] );
    
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );
    
    int listenfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert( listenfd >= 0 );
    
    int ret = bind( listenfd, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret != -1 );
    
    ret = listen( listenfd, 5 );
    assert( ret != -1 );
    
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof( client_address );
    int fd = accept( listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
    if( fd < 0 ){
        printf( "errno is: %d\n", errno );
    }
    else{
        char buffer[ BUFFER_SIZE ];             // 接收缓冲区
        memset( buffer, '\0', BUFFER_SIZE );
        int data_read = 0;                    
        int read_index = 0;                     // 当前已经读取了多少字节的客户数据
        int checked_index = 0;                  // 当前已经分析完了多少字节的客户数据
        int start_line = 0;                     // 在接收缓冲区中的起始位置

        CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;   // 设置主状态机的初识状态-分析请求行
        while( 1 ){// 循环读取数据并进行分析

            data_read = recv( fd, buffer + read_index, BUFFER_SIZE - read_index, 0 );
            if ( data_read == -1 ){
                printf( "reading failed\n" );
                break;
            }
            else if ( data_read == 0 ){
                printf( "remote client has closed the connection\n" );
                break;
            }
    
            read_index += data_read;// 更新当前读取数据的数量
            HTTP_CODE result = parse_content( buffer, checked_index, checkstate, read_index, start_line );// 
            if( result == NO_REQUEST ){
                continue;
            }else if( result == GET_REQUEST ){
                send( fd, szret[0], strlen( szret[0] ), 0 );
                break;
            }else{
                send( fd, szret[1], strlen( szret[1] ), 0 );
                break;
            }
        }
        close( fd );
    }
    
    close( listenfd );
    return 0;
}
