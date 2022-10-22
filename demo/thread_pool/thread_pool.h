#ifndef THREADPOLL_H
#define THREADPOLL_H

#include <pthread.h>
#include <list>
#include <exception>
#include "locker.h"
#include <cstdio>

// 线程池类，定义成模板类是为了代码的复用。
// 模板参数T是任务类
// 基类任务-具体执行子类任务
template<typename T>
class thread_pool{
public:
    // 默认线程数量与请求队列数量
    thread_pool(int thread_number = 8,int max_request = 1000);
    ~thread_pool();
    bool append(T* request);// 添加任务

private:
    static void* worker(void* arg);// 执行函数,需要注意的是，静态成员，无法访问非静态成员。

    void run();
private:
    // 线程的数量
    int m_thread_number;

    // 线程池数组，大小为m_thread_number
    pthread_t* m_threads;

    // 请求队列中最多允许等待处理的请求数量。
    int m_max_requests;

    // 请求队列-装任务
    std::list<T*>m_workqueue;

    // 互斥锁
    locker m_queue_locker;

    // 信号量-用来判断是否有任务需要进行处理
    // 开始我还没搞懂信号量是来干jb的，原来是，通过信号量来控制线程的执行，即同上讲的，当前是否有需要处理的任务。
    sem m_queue_stat;   

    // 是否结束线程，结束掉某一个线程
    bool m_stop; 

};

template<typename T>
thread_pool<T>::thread_pool(int thread_number,int max_requests):
    m_thread_number(thread_number),
    m_max_requests(max_requests),
    m_stop(false),
    m_threads(NULL){
        if((thread_number <= 0) || (max_requests <= 0)){
            throw std::exception();
        }
        m_threads = new pthread_t[m_thread_number];// 创建线程数组
        if(!m_threads){// 线程数组创建失败
            throw std::exception();
        }
        // 给创建出来的线程设置线程脱离属性，子线程自己销毁资源
        for(int i = 0; i < m_thread_number;i++){
            printf("create the %d thread\n",i);
            // 线程的执行函数必须为静态的
            // 把当前类对象，传递过去，以解决静态成员无法调用非静态成员的问题。
            if(pthread_create(m_threads+i,NULL,worker,this) != 0){
                delete [] m_threads;
                throw std::exception();
            }
            // 设置线程分离
            if(pthread_detach(m_threads[i])){
                delete []m_threads;
                throw std::exception();
            }

            
        }
    }


template<typename T>
thread_pool<T>::~thread_pool(){
    delete []m_threads;
    m_stop = true;
}

template<typename T>
bool thread_pool<T>::append(T* request){// 在请求队列中添加任务
    
    m_queue_locker.lock();
    if(m_workqueue.size() > m_max_requests){// 超过请求队列中可允许等待的最大数量
        m_queue_locker.unlock();
        return false;
    }
    m_workqueue.push_back(request);// 将要添加的任务追加到后面
    m_queue_locker.unlock();
    m_queue_stat.post();// 增加一个任务，信号量加一，应该是通过信号量来进行多线程之间任务的选取。
    return true;
}

template<typename T>
void* thread_pool<T>::worker(void* arg){
    thread_pool* pool = (thread_pool*)arg;
    pool->run();
    return pool;
}

template<typename T>
void thread_pool<T>::run(){
    while (!m_stop){
        m_queue_stat.wait();// 看看有没有任务可以做
        m_queue_locker.lock();
        if(m_workqueue.empty()){// 看看队列是不是空的，这步可以仔细相下是否有用。
            m_queue_locker.unlock();
            continue;
        }
        T* request = m_workqueue.front();//获取第一个任务
        m_workqueue.pop_front();
        m_queue_locker.unlock();

        if(!request){
            continue;
        }

        // 执行任务
        request->pocess();
    }
    
}

#endif