#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<assert.h>
#include<fcntl.h>
#include<unistd.h>

//因为下面的函数指针所以单独拿出来声明typedef
typedef struct _ConnectStat  ConnectStat;

typedef void(*response_handler) (ConnectStat * stat);

// 保存链接状态的结构体
struct _ConnectStat {
	int fd;						//句柄
	char name[64];				//姓名
	char  age[64];				//年龄
	struct epoll_event _ev;		//当前文件句柄对应epoll事件
	int  status;				//0-未登录，1-已登录
	response_handler handler;	//不同页面的处理函数
};

//http协议相关代码
ConnectStat * stat_init(int fd);
void connect_handle(int new_fd);
void do_http_respone(ConnectStat * stat);
void do_http_request(ConnectStat * stat);
void welcome_response_handler(ConnectStat * stat);
void commit_respone_handler(ConnectStat * stat);


const char *main_header = "HTTP/1.0 200 OK\r\nServer: Xuanxuan Server\r\nContent-Type: text/html\r\nConnection: Close\r\n";

static int epfd = 0;

void usage(const char* argv)
{
	printf("%s:[ip][port]\n", argv);
}

// 将fd-设置为非阻塞状态，即给指定fd添加状态
void set_nonblock(int fd){
	// 这里的文件状态标志flag即open函数的第二个参数
	int fl = fcntl(fd, F_GETFL);// F_GETFL获取文件描述符的文件状态标志flag
	fcntl(fd, F_SETFL, fl | O_NONBLOCK);// 添加flag-非阻塞方式

	// fcntl函数 https://blog.csdn.net/zhoulaowu/article/details/14057799
	// O_NONBLOCK https://blog.csdn.net/cjfeii/article/details/115484558
}

// 创建一个套接字 - 略
int startup(char* _ip, int _port){
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0){
		perror("sock");
		exit(2);
	}

	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in local;
	local.sin_port = htons(_port);
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = inet_addr(_ip);

	if (bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0){
		perror("bind");
		exit(3);
	}

	if (listen(sock, 5) < 0){
		perror("listen");
		exit(4);
	}
	return sock;    //返回套接字
}


// 初始化链接状态结构体
ConnectStat * stat_init(int fd) {
	ConnectStat * temp = NULL;
	temp = (ConnectStat *)malloc(sizeof(ConnectStat));

	if (!temp) {
		fprintf(stderr, "malloc failed. reason: %m\n");
		return NULL;
	}

	memset(temp, '\0', sizeof(ConnectStat));
	temp->fd = fd; 
	temp->status = 0; 

}

//初始化连接，然后等待浏览器发送请求
void connect_handle(int new_fd) {
	ConnectStat *stat = stat_init(new_fd);
	set_nonblock(new_fd);

	stat->_ev.events = EPOLLIN;//关心读事件-客户端来读(请求)
	stat->_ev.data.ptr = stat;

	epoll_ctl(epfd, EPOLL_CTL_ADD, new_fd, &stat->_ev);    //交给epoll监视

}

// 进行响应
void do_http_respone(ConnectStat * stat) {
	stat->handler(stat);// 使用函数指针调用设置的页面处理函数
}

// 解析http请求
void do_http_request(ConnectStat * stat) {

	//读取和解析http 请求
	char buf[4096];
	char * pos = NULL;
	//while  header \r\n\r\ndata
	ssize_t _s = read(stat->fd, buf, sizeof(buf) - 1);
	if (_s > 0)
	{
		buf[_s] = '\0';
		
		printf("receive from client:%s\n", buf);

		pos = buf;

		//Demo 仅仅演示效果，不做详细的协议解析
		if (!strncasecmp(pos, "GET", 3)) {
			stat->handler = welcome_response_handler;
		}
		else if (!strncasecmp(pos, "Post", 4)) {
			//获取 uri
			printf("---Post----\n");
			pos += strlen("Post");
			while (*pos == ' ' || *pos == '/') ++pos;

			if (!strncasecmp(pos, "commit", 6)) {//获取名字和年龄
				int len = 0;

				printf("post commit --------\n");
				pos = strstr(buf, "\r\n\r\n");
				char *end = NULL;
				if (end = strstr(pos, "name=")) {
					pos = end + strlen("name=");
					end = pos;
					while (('a' <= *end && *end <= 'z') || ('A' <= *end && *end <= 'Z') || ('0' <= *end && *end <= '9'))	end++;
					len = end - pos;
					if (len > 0) {
						memcpy(stat->name, pos, end - pos);
						stat->name[len] = '\0';
					}
				}

				if (end = strstr(pos, "age=")) {
					pos = end + strlen("age=");
					end = pos;
					while ('0' <= *end && *end <= '9')	end++;
					len = end - pos;
					if (len > 0) {
						memcpy(stat->age, pos, end - pos);
						stat->age[len] = '\0';
					}
				}
				stat->handler = commit_respone_handler;

			}
			else {
				stat->handler = welcome_response_handler;
			}

		}
		else {
			stat->handler = welcome_response_handler;
		}

		//生成处理结果 html ,write

		stat->_ev.events = EPOLLOUT;
		//stat->_ev.data.ptr = stat;
		epoll_ctl(epfd, EPOLL_CTL_MOD, stat->fd, &stat->_ev);   //二次托管
	}else if (_s == 0){//client:close
		printf("client: %d close\n", stat->fd);
		epoll_ctl(epfd, EPOLL_CTL_DEL, stat->fd, NULL);
		close(stat->fd);
		free(stat);
	}else{
		perror("read");
	}

}

// 直接访问给出的响应-write给stat-fd
void welcome_response_handler(ConnectStat * stat) {
	const char * welcome_content = "\
			<html lang=\"zh-CN\">\n\
			<head>\n\
			<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">\n\
			<title>This is a test</title>\n\
			</head>\n\
			<body>\n\
			<div align=center height=\"500px\" >\n\
			<br/><br/><br/>\n\
			<h2>Hello World</h2><br/><br/>\n\
			<form action=\"commit\" method=\"post\">\n\
			姓名: <input type=\"text\" name=\"name\" />\n\
			<br/>年龄: <input type=\"password\" name=\"age\" />\n\
			<br/><br/><br/><input type=\"submit\" value=\"提交\" />\n\
			<input type=\"reset\" value=\"重置\" />\n\
			</form>\n\
			</div>\n\
			</body>\n\
			</html>";

	char sendbuffer[4096];
	char content_len[64];

	strcpy(sendbuffer, main_header);
	snprintf(content_len, 64, "Content-Length: %d\r\n\r\n", (int)strlen(welcome_content));
	strcat(sendbuffer, content_len);
	strcat(sendbuffer, welcome_content);
	//printf("send reply to client \n%s", sendbuffer);

	// 写给客户端-即发起请求的浏览器
	write(stat->fd, sendbuffer, strlen(sendbuffer));

	stat->_ev.events = EPOLLIN;
	//stat->_ev.data.ptr = stat;
	epoll_ctl(epfd, EPOLL_CTL_MOD, stat->fd, &stat->_ev);


}


// 点击commit后给出的响应
void commit_respone_handler(ConnectStat * stat) {
	const char * commit_content = "\
		<html lang=\"zh-CN\">\n\
		<head>\n\
		<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">\n\
		<title>This is a test</title>\n\
		</head>\n\
		<body>\n\
		<div align=center height=\"500px\" >\n\
		<br/><br/><br/>\n\
		<h2>欢迎&nbsp;%s &nbsp;,年龄&nbsp;%s！</h2><br/><br/>\n\
		</div>\n\
		</body>\n\
		</html>\n";

	char sendbuffer[4096];
	char content[4096];
	char content_len[64];
	int len = 0;

	len = snprintf(content, 4096, commit_content, stat->name, stat->age);
	strcpy(sendbuffer, main_header);
	snprintf(content_len, 64, "Content-Length: %d\r\n\r\n", len);
	strcat(sendbuffer, content_len);
	strcat(sendbuffer, content);
	printf("send reply to client \n%s", sendbuffer);

	write(stat->fd, sendbuffer, strlen(sendbuffer));

	stat->_ev.events = EPOLLIN; // 读事件-客户端fd来读

	epoll_ctl(epfd, EPOLL_CTL_MOD, stat->fd, &stat->_ev);// 交给epoll来监视
}
