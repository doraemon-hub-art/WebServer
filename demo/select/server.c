#include <sys/types.h> 
#include <sys/socket.h> 
#include <stdio.h> 
#include <netinet/in.h> 
#include <sys/time.h> 
#include <sys/ioctl.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <errno.h>

int errno;
int main(void){
    int server_sockfd,client_sockfd;
    int server_len,client_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    int result;
    //两个文件描述符集合
    fd_set readfds,testfds;//readfds用于检测输出是否就绪的文件描述符集合

    server_sockfd = socket(AF_INET,SOCK_STREAM,0); // 建立服务端socket
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(9000);
    server_len = sizeof(server_address);
    bind(server_sockfd,(struct sockaddr*)&server_address,server_len);
    listen(server_sockfd,5);// 监听队列最多容纳5个
    
    FD_ZERO(&readfds);// 清空置0
    FD_SET(server_sockfd,&readfds);// 将服务端socket加入到集合中

    /*
    int select(int nfds, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval *timeout);
    */ 

    while(1){
        char ch;
        int fd;
        int nread;
        testfds = readfds;//相当于备份一份，因为调用select后，传进去的文件描述符集合会被修改。
        struct timeval my_time;
        my_time.tv_sec = 2;
        my_time.tv_usec = 0;
        printf("server waiting\n");
        // 监视server_sockfd与client_sockfd
        //result = select(FD_SETSIZE,&testfds,(fd_set*)0,(fd_set*)0,(struct timeval* )0); //无限期阻塞，并测试文件描述符变动
        result = select(FD_SETSIZE,&testfds,(fd_set*)0,(fd_set*)0,&my_time); //根据my_time中设置的时间进行等待,超过继续往下执行。
        if(result < 0){//有错误发生
            perror("select errno"); 
            exit(1);
        }else if(result == 0){//超过等待时间，未响应    
            FD_ZERO(&readfds);// 清空置0
            FD_SET(server_sockfd,&readfds);// 将服务端socket重新加入到集合中
            printf("no connect request \n");
            continue;//没有响应的就别下去遍历了
        }
        //扫描所有的文件描述符(遍历所有的文件句柄)，是件很耗时的事情，严重拉低效率。
        for(fd = 0;fd<FD_SETSIZE;fd++){
            //找到相关文件描述符，判断是否在testfds这个文件描述符集合中。 
            if(FD_ISSET(fd,&testfds)){
                //判断是否为服务器套接字，是则表示为客户端请求连接
                if(fd == server_sockfd){
                    client_len = sizeof(client_address);
                    client_sockfd = accept(server_sockfd,(struct sockaddr*)&client_address,&client_len);
                    FD_SET(client_sockfd,&readfds);//将客户端socket加入到集合中，用来监听是否有数据来。
                    printf("adding client on fd %d\n",client_sockfd);;
                }else{// 客户端来消息了
                    //获取接收缓存区中的字节数
                    ioctl(fd,FIONREAD,&nread);//即获取fd来了多少数据

                    //客户端数据请求完毕，关闭套接字，并从集合中清除相应的套接字描述符
                    if(nread ==0){
                        close(fd);
                        FD_CLR(fd,&readfds);//去掉关闭的fd
                        printf("removing client on fd %d\n", fd);
                    }else{//处理客户数请求
                        read(fd,&ch,1);
                        sleep(5);
                        printf("serving client on fd %d\n", fd);
                        ch++;
                        write(fd, &ch, 1);
                    }
                }
            }
        }
    }
    return 0;
}