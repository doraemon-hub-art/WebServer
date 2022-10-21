#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "locker.h"
#include "thread_pool.h"
#include <libgen.h>
#include <signal.h>
#include <string.h>

// 添加信号捕捉
void addsig(int sig,void(handler)(int)){
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));// 置空
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);// 设计值信号屏蔽集
    sigaction(sig,&sa,NULL);
}

int main(int argc,char* argv[]){

    if(argc <= 1){
        printf("按照如下格式运行:%s ,port_number\n",basename(argv[0]));
        exit(-1);
    }
    // 获取端口号
    int port = atoi(argv[1]);

    return 0;
}