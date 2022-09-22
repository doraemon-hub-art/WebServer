#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>

#define MAX_FD  8192 //最大文件标识符
struct pollfd  fds[MAX_FD];
int cur_max_fd = 0;     //当前要监听的最大文件描述符+1，减少要遍历的数量。

int main(void){

    int server_sockfd,client_sockfd;
    int server_len,client_len;
    struct sockaddr_in server_address,client_address;
    int result;
    server_sockfd = socket(AF_INET,SOCK_STREAM,0);//服务端socket
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(9000);
    server_len = sizeof(server_address);
    bind(server_sockfd,(struct sockaddr*)&server_address,server_len);
    listen(server_sockfd,5);

    //添加待监测文件描述符到fds数组中
    fds[server_sockfd].fd = server_sockfd;
    fds[server_sockfd].events = POLLIN;
    fds[server_sockfd].revents = 0;
    
    if(cur_max_fd <= server_sockfd){
        cur_max_fd = server_sockfd+1;
    }

    while(1){
        char ch;
        int i,fd;
        int nread;
        printf("server waiting\n");

        result = poll(fds,cur_max_fd,1000);
        if(result <0){
            perror("server5");
            exit(1);
        }else if(result == 0){
            printf("no connect,end waiting\n");
        }else{//大于0，返回的是fds中处于就绪态的文件描述符个数。

        }
        //扫描文件描述符
        for(i = 0; i < cur_max_fd;i++){
            if(fds[i].revents){//有没有结果,没有结果说明该文件描述符上还未发生事件。
                fd= fds[i].fd;
                //判断是否为服务器套接字，是则表示为客户端请求连接
                if(fd == server_sockfd){
                    client_len = sizeof(client_address);
                    client_sockfd = accept(server_sockfd,(struct sockaddr*)&client_address,&client_len);
                    fds[client_sockfd].fd = client_sockfd;
                    fds[client_sockfd].events = POLLIN;
                    fds[client_sockfd].revents = 0;
                    if(cur_max_fd <= client_sockfd){
                        cur_max_fd = client_sockfd + 1;
                    }
                    printf("adding client on fd %d\n",client_sockfd);
                }else{//客户端socket中有数据请求
                    if(fds[i].revents & POLLIN){//读
                        nread = read(fd,&ch,1);
                        if(nread == 0){
                            close(fd);
                            memset(&fds[i],0,sizeof(struct pollfd));
                            printf("removing client on fd %d\n",fd);
                        }else{//写
                            sleep(5);
                            printf("serving client on fd %d,receive: %c\n",fd,ch);
                            ch++;
                            fds[i].events = POLLOUT;//添加一个写事件监听
                        }
                    }else if(fds[i].revents & POLLOUT){//写
                        write(fd,&ch,1);
                        fds[i].events = POLLIN;
                    }

                }
            }
        }

    }
    return 0;
}
