/**
 * @file work_thread.h
 * @author {gangx} ({gangx6906@gmail.com})
 * @brief 工作线程管理容器
 * @version 0.1
 * @date 2024-04-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#ifndef WORK_THREAD_H
#define WORK_THREAD_H
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
/**
 * @brief 工作线程的容器
 * 
 */
typedef struct WorkThreadsContainer{
    size_t max_num;//最大线程数目
    size_t min_num;//最少线程数目
    size_t busy_num;//在工作的线程数目
    size_t live_num;//现在存货的线程数目
    size_t exit_num;//要销毁的线程数目
    pthread_t *thread_ids;//所有工作线程的id
    pthread_mutex_t mutext_busy;//操作busyNum的互斥锁

}WorkThreadsContainer;
/**
 * @brief 工作线程执行的工作函数
 * 
 * @param arg 实际上是线程池 
 * @return void* 
 */
void*work(void*arg);
/**
 * @brief 创建一个工作线程的容器
 * 
 * @param min 
 * @param max 
 * @return WorkThreadsContainer* 
 */
WorkThreadsContainer* init_work_threads_container(size_t min,size_t max){
    WorkThreadsContainer* container = (WorkThreadsContainer*)malloc(sizeof(WorkThreadsContainer));
    container->max_num = max;
    container->min_num = min;
    container->busy_num = 0;
    container->exit_num = 0;
    container->live_num = min;
    container->thread_ids = (pthread_t*)malloc(sizeof(pthread_t)*max);
    memset(container->thread_ids,0,sizeof(pthread_t)*max);//全部初始化为0
    return container;
}
#endif