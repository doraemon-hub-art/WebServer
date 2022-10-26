#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "../common/common.h"

template<typename T>
class thread_pool{

public:
    thread_pool(int actor_model,);
    ~thread_pool();

private:
    int m_thread_number;    // 线程池中的线程数
    int m_max_requests;     // 请求队列中允许的最大请求数
    pthread_t* m_threads;   // 描述线程池的数组(线程数组)
    

};





#endif