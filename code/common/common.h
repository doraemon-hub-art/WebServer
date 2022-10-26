#ifndef COMMON_H
#define COMMON_H

#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/uio.h>
#include <libgen.h>
#include <string.h>
#include <exception>
#include <semaphore.h>  
#include <pthread.h>
#include <iostream>


static const int MAX_FD = 65535;// 最大文件描述符
static const int MAX_EVENT_NUMBER = 10000; // 最大事件数






#endif