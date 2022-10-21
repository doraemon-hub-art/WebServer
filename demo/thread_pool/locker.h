#ifndef LOCKER_H
#define LOCKER_H

#include <pthread.h>
#include <exception>
#include 

// 线程同步机制封装类

// 互斥锁类
class locker{

public:
    locker(){
        if(pthread_mutex_init(&m_mutex,NULL) != 0){
            throw std::exception();
        }
    }

    ~locker(){
        pthread_mutex_destroy(&m_mutex);
    }

    bool lock(){
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    
    bool unlock(){
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

    pthread_mutex_t* get(){
        return  &m_mutex;
    }
private:
    pthread_mutex_t m_mutex;
};

// 条件变量类
class cond{
public:
    cond(){
        if(pthread_cond_init(&m_cond,NULL) != 0){
            throw std::exception();
        }
    }

    ~cond(){
        pthread_cond_destroy(&m_cond);
    }
    
    bool timewait(pthread_mutex_t* mutex,struct timespec t){
        return pthread_cond_timedwait(&m_cond,mutex,&t) == 0;
    }

    bool signal(){// 唤醒一个的等待线程
        return pthread_cond_signal(&m_cond);
    }

    bool boradcast(){// 唤醒所有等待线程
        return pthread_cond_broadcast(&m_cond) == 0;
    }
private:
    pthread_cond_t m_cond;  
};

// 信号量类
class sem{
public:

private:

};




#endif