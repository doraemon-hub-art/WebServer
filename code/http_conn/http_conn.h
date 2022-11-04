#include "../common/common.h"
#include "../sql_connection_pool/sql_connection_pool.h"
#include "../lst_timer/lst_timer.h"
#include "../log/log.h"

class http_conn{

public:
    // 设置读取文件的名称m_read_file大小
    static const int FILE_NAME_LEN = 200;
    // 设置读缓冲区m_read_buf大小
    static const int READ_BUFFER_SIZE = 2048;
    // 设置写缓冲区m_write_buf大小
    static const int WRITE_BUFFER_SIZE = 1024;

    enum METHOD{// http方法
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };

    enum CHECK_STATE{// 主状态机状态
        CHECK_STATE_REQUESTLINE = 0,// 解析请求行
        CHECK_STATE_HEADER,// 解析请求头
        CHECK_STATE_CONTENT// 解析请求内容
    };

    enum HTTP_CODE{// 报文解析的结果
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    enum LINE_STATUS{// 从状态机的状态
        LINE_OK = 0,    //  成功读取到一行
        LINE_BAD,   // 当前行数据出错
        LINE_OPEN   // 数据不完整
    };

public:

    http_conn(){}
    ~http_conn(){}

public:

    // 初始化
    void init(int socked,const sockaddr_in &addr,char*,
        int,int,std::string user,std::string passwd,
        std::string sqlname);

    // 关闭http连接
    void close_conn(bool real_close = true);

    // 
    void process();

    // 读取浏览器发来的全部数据
    bool read_once();

    // 响应报文写入
    bool write();

    sockaddr_in* get_address(){
        return &m_address;
    }
};