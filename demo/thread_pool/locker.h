#ifndef LOCKER_H
#define LOCKER_H

#include <pthread.h>
#include <exception>

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

    }
private:
    pthread_cond_t m_cond;
}




#endif