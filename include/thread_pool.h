/**
 * @file thread_pool.h
 * @author {gangx} ({gangx6906@gmail.com})
 * @brief 线程池对任务和执行任务的线程进行管理
 * @version 0.1
 * @date 2024-04-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include "task_queue.h"
#include "work_thread.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
typedef struct ThreadPool{
    TaskQueue *task_queue;//任务管理的任务队列
    WorkThreadsContainer *work_threads;//工作线程容器
    pthread_t manager_thread_id;//管理工作线程的线程id
    int shut_down;//是否销毁整个线程池，停止运行 
    pthread_mutex_t muetex_pool;//操作整个线程池的互斥锁
}ThreadPool;

/**
 * @brief 初始化一个线程池容器
 * 
 * @param min 
 * @param max 
 * @return ThreadPool* 
 */
ThreadPool* init_thread_pool(size_t min,size_t max){
    //初始化对应的子容器并且设置
    WorkThreadsContainer* container =  init_work_threads_container(min,max);
    TaskQueue* task_queue = init_task_queue();
    ThreadPool * thread_pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    thread_pool->task_queue  = task_queue;
    thread_pool->work_threads = container;
    //创建开始的线程
    for (int i = 0; i < container->min_num; i++)
    {
        pthread_create(&container->thread_ids[i],NULL,work,thread_pool);
    }
    return thread_pool;
}
/**
 * @brief 销毁一个线程池
 * 
 * @param pool 
 * @return int 函数执行结构 -1失败 0成功
 */
int destroy_thread_pool(ThreadPool *pool){
    if (pool==NULL)
    {
        return -1;
    }
    pool->shut_down = 1;//这里只有这种情况操作shut_down所以不使用锁
    pthread_join(pool->manager_thread_id,NULL);//先等待管理线程结束
    //注销现有线程
    for (int i = 0; i < pool->work_threads->live_num; i++)//增加现存线程次的条件变量来删除剩余的线程
    {
        pthread_cond_signal(&pool->task_queue->notEmpty);   
    }
    //注销任务队列
    destroy_task_queue(pool->task_queue);
    //注销锁和条件变量
    pthread_mutex_destroy(&pool->work_threads->mutext_busy);
    pthread_mutex_destroy(&pool->muetex_pool);
    pthread_cond_destroy(&pool->task_queue->notEmpty);
    /**
     * 对于多重的指针释放，一个从底层开始释放到最高层
     * 
     */
    //注销线程列表空间
    free(pool->work_threads->thread_ids);
    //注销工作容器
    free(pool->work_threads);
    //注销线程池
    free(pool);
    return 0;
}
/**
 * @brief 添加任务到线程池里面
 * 
 * @param thread_pool 
 * @param func 
 * @param arg 
 */
void thread_pool_add_task(ThreadPool*thread_pool,void*(*func)(void*),void*arg){
    if (thread_pool->shut_down)
    {
        //线程池要注销就不必添加了
        return;
    }
    Task * task = (Task*)malloc(sizeof(Task));
    task->function = func;
    task->arg = arg;
    pthread_mutex_lock(&thread_pool->muetex_pool);
    queue(thread_pool->task_queue,task);
    pthread_cond_signal(&thread_pool->task_queue->notEmpty);//添加条件变量，这里是添加了任务之后再添加的条件变量此时工作的线程就会来执行任务而不是自身线程的注销
    pthread_mutex_unlock(&thread_pool->muetex_pool);
}
/**
 * @brief Get the thread pool live thread num object
 * 
 * @param pool 
 * @return size_t 
 */
size_t get_thread_pool_live_thread_num(ThreadPool*pool){
    pthread_mutex_lock(&pool->muetex_pool);
    size_t live = pool->work_threads->live_num;
    pthread_mutex_unlock(&pool->muetex_pool);
    return live;
}
/**
 * @brief Get the thread pool busy thread num object
 * 
 * @param pool 
 * @return size_t 
 */
size_t get_thread_pool_busy_thread_num(ThreadPool*pool){
    pthread_mutex_lock(&pool->work_threads->mutext_busy);
    size_t busy = pool->work_threads->busy_num;
    pthread_mutex_unlock(&pool->work_threads->mutext_busy);
    return busy;
}
void*work(void*arg){
   ThreadPool *pool = (ThreadPool*)arg;
   while (1)//不断的执行任务
   {
    pthread_mutex_lock(&pool->muetex_pool);
    while (pool->task_queue->size==0&&!pool->shut_down)//如果线程池还在运行，没有可以执行的任务就阻塞等待
    {
        pthread_cond_wait(&pool->task_queue->notEmpty,&pool->muetex_pool);//等待任务添加，等待期间解锁对线程池的锁
        if (pool->work_threads->exit_num>0)//如果线程池需要删除一些线程，那就删除
        {
            pool->work_threads->exit_num--;
            if (pool->work_threads->live_num>pool->work_threads->min_num)
            {
                pool->work_threads->live_num--;
                for (int i = 0; i < pool->work_threads->max_num; i++)
                {
                    if (pool->work_threads->thread_ids[i]==pthread_self())
                    {
                        printf("线程 %ld 注销\n",pthread_self());
                        pool->work_threads->thread_ids[i]=0;//线程id归零
                        break;
                    }
                }
                pthread_mutex_unlock(&pool->muetex_pool);//释放对线程池操作的锁
                pthread_exit(pool);//结束线程
            }
        } 
    }
    //在我们等待任务队列添加任务可能执行了对线程池的注销所有再判断线程池是否注销了？
    if (pool->shut_down)
    {
        pthread_mutex_unlock(&pool->muetex_pool);
        pthread_exit(pool);
    }
    //线程池依然存在有任务来执行并且不需要删除线程就执行任务
    Task *task = dequeue(pool->task_queue);//从任务队列取出任务
    pthread_mutex_unlock(&pool->muetex_pool);//拿到了要执行的任务就不再对线程池进行访问，解锁
    printf("线程 %ld 开始工作\n",pthread_self());
    pthread_mutex_lock(&pool->work_threads->mutext_busy);//执行操作要对线程池中的执行任务线程的数目进行修改
    pool->work_threads->busy_num++;//执行任务添加繁忙线程数目
    pthread_mutex_unlock(&pool->work_threads->mutext_busy);//先解除对繁忙线程数目的锁避免因为执行的任务耗时过长导致其他工作线程不能操作修改繁忙线程数
    //进行函数的执行
    task->function(task->arg);//执行
    free(task->arg);//arg属性是执行的任务函数的参数所以在执行函数之后就可以释放了
    //再修改线程池的繁忙线程数
    pthread_mutex_lock(&pool->work_threads->mutext_busy);
    pool->work_threads->busy_num--;
    pthread_mutex_unlock(&pool->work_threads->mutext_busy); 
   }
   return NULL;
}
/**
 * @brief 管理工作线程的线程应该执行函数，该函数的逻辑是根据现在的任务和线程供应情况来动态对线程进行调整
 * 
 * @param arg 
 * @return void* 
 */
void*manager(void*arg){
    ThreadPool *pool = (ThreadPool*)arg;
    while (!pool->shut_down)//线程池没有销毁就应该不断运行
    {
        sleep(1);//每一秒就检测当前线程池的状态对工作线程进行调整
        pthread_mutex_lock(&pool->muetex_pool);//获取对线程池的操作锁
        size_t live = pool->work_threads->live_num;//获取存活的线程
        size_t task_num = pool->task_queue->size;//获取当前需要完成的任务数目
        pthread_mutex_unlock(&pool->muetex_pool);//解除线程池的操作锁
        pthread_mutex_lock(&pool->work_threads->mutext_busy);//获取访问处理任务的线程变量锁
        size_t busy = pool->work_threads->busy_num;
        pthread_mutex_unlock(&pool->work_threads->mutext_busy);//解除访问处理任务的线程变量锁
        /**
         * 线程池调整线程数目的策略
         * 添加线程的条件是：有空间添加（存活的线程数目小于最大线程数目），需要执行的任务数目较多（任务队列存在的任务数目大于存活的线程）
         * 删除线程的条件是：线程没有充分的使用（在进行工作的线程数目小于现在存活的线程数目的一半），还有删除的空间（当前的存活的线程大于最小线程的数目）
         */
        //添加线程
        if (live<pool->work_threads->max_num&&task_num>live)
        {
            //确认应该添加的线程数目，这里设置为task_num-live
            size_t add_thread_num = task_num - live;
            //添加线程的逻辑，应该是遍历整个线程id列表来寻找到没有创建的线程位置来创建这里添加的线程数目，处理条件是还有线程空位（遍历）并且还有添加的线程数目（要添加的线程数目大于0）
            for (int i = 0; i < pool->work_threads->max_num&&add_thread_num>0; i++)
            {
                if(pool->work_threads->thread_ids[i]==0){//这是一个线程空位可以创建线程
                    pthread_create(&pool->work_threads->thread_ids[i],NULL,work,pool);//创建线程
                    add_thread_num--;
                }
            }
        }
        //销毁线程
        if (live>pool->work_threads->min_num&&busy<live/2)
        {
            pthread_mutex_lock(&pool->muetex_pool);//获取对线程池的操作锁
            //确认销毁数目，这里设置为live/2-busy
            pool->work_threads->exit_num = live/2-busy;//设置要注销的线程数目
            pthread_mutex_unlock(&pool->muetex_pool);//解除线程池的操作锁
            for (int i = 0; i < live/2-busy; i++)
            {
                pthread_cond_signal(&pool->task_queue->notEmpty);//发送条件变量，这样没有添加任务发送信号会让工作线程执行销毁线程的逻辑。     
            }
        }   
    }   
}
#endif