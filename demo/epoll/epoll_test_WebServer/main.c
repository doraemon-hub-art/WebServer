#include "epoll_server.h"

int main(int argc, char *argv[]){

	if (argc != 3){//检查输入的参数个数是否正确
		usage(argv[0]);
		exit(1);
	}
	
	//创建一个server socket
	int listen_sock = startup(argv[1], atoi(argv[2]));      

	//创建epoll
	epfd = epoll_create(256);
	if (epfd < 0){//创建失败
		perror("epoll_create");
		exit(5);
	}

	ConnectStat * stat = stat_init(listen_sock);// 自定义数据存储

	struct epoll_event _ev; 	//epoll事件结构体
	_ev.events = EPOLLIN;    	//设置关心事件为读事件     
	_ev.data.ptr = stat;    	//接收返回值
	

    //将listen_sock添加到epfd中，关心读事件，有客户端来请求链接
	epoll_ctl(epfd, EPOLL_CTL_ADD, listen_sock, &_ev);

	struct epoll_event revs[64];//接收返回的产生响应的事件

	int timeout = -1;// -1无限期阻塞
	int num = 0;// 就绪的请求I/O个数

	while (1){
		//检测事件
		switch ((num = epoll_wait(epfd, revs, 64, timeout))){
		case 0:   //监听超时               
			printf("timeout\n");
			break;
		case -1: //出错     
			perror("epoll_wait");
			break;
		default:{	//>0，即返回了需要处理事件的数目
			//拿到对应的文件描述符
			struct sockaddr_in peer;
			socklen_t len = sizeof(peer);

			for (int i = 0; i < num; i++){//
                //拿到该fd相关的链接信息
				ConnectStat * stat = (ConnectStat *)revs[i].data.ptr;

				int rsock = stat->fd;//拿到对应的fd，进行如下的判断
				if (rsock == listen_sock && (revs[i].events) && EPOLLIN) {// 有客户端链接
					int new_fd = accept(listen_sock, (struct sockaddr*)&peer, &len);
					
					if (new_fd > 0){//accept成功
						printf("get a new client:%s:%d\n", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));	
						connect_handle(new_fd);// 监听新进来的客户端fd
					}
				}else {//除server socket 之外的其他fd就绪
					if (revs[i].events & EPOLLIN){//有数据可读
						do_http_request((ConnectStat *)revs[i].data.ptr);
					}else if (revs[i].events & EPOLLOUT){//写
						do_http_respone((ConnectStat *)revs[i].data.ptr);// 完成响应后会再次关心EPOLLIN事件，等待下一次请求。					
					}else{
					}
				}
			}
		}
		break;
		}
	}
	return 0;
}


