/**
 * @file task_queue.h
 * @author {gangx} ({gangx6906@gmail.com})
 * @brief 任务队列，程序需要执行的操作（函数）的队列
 * @version 0.1
 * @date 2024-04-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H
#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
/**
 * @brief 任务结构体包含了任务要执行的函数和函数的参数
 * 
 */
typedef struct Task{
    void*(*function)(void*arg);
    void*arg;
}Task;
/**
 * @brief 任务队列的节点
 * 
 */
typedef struct TaskQueueNode{
    Task*task;
    struct TaskQueueNode *next;
    struct TaskQueueNode *pre;
    
}TaskQueueNode;
/**
 * @brief 任务队列
 * 
 */
typedef struct TaskQueue{
    TaskQueueNode*head;
    TaskQueueNode*tail;
    size_t size;
    pthread_cond_t notEmpty;//队列不为空的条件变量，用于控制任务的添加执行以及对工作线程进行删除和创建
}TaskQueue;
/**
 * @brief 初始化一个空的任务队列
 * 
 * @return TaskQueue* 
 */
TaskQueue* init_task_queue(){
    TaskQueue *task_queue = (TaskQueue*)malloc(sizeof(TaskQueue));
    task_queue->size = 0;
    task_queue->head = NULL;
    task_queue->tail = NULL;
}
/**
 * @brief  queue 添加任务到任务队列里面
 * 
 * @param task_queue 
 * @param task 
 */
void queue(TaskQueue*task_queue,Task*task){
    if (task==NULL)
    {
        fputs("task is null\n",stderr);
        return;    
    }
    if (task_queue->size==0)//空队列的处理
    {
        TaskQueueNode * head_node = (TaskQueueNode*)malloc(sizeof(TaskQueueNode));
        head_node->task = task;
        head_node->next=NULL;
        head_node->pre = NULL;
        task_queue->head = head_node;
        task_queue->tail = head_node;   
    }
    else{//非空队列的处理
        TaskQueueNode*tail = task_queue->tail;//获取队列的最后一个节点指针
        TaskQueueNode*add_node = (TaskQueueNode*)malloc(sizeof(TaskQueueNode));
        add_node->task = task;
        add_node->next = NULL;
        add_node->pre = tail;
        tail->next = add_node;
        task_queue->tail = add_node;
    }
    task_queue->size++;
}
/**
 * @brief 任务出任务队列
 * 
 * @param task_queue 
 * @return Task* 
 */
Task* dequeue(TaskQueue*task_queue){
    if (task_queue->size==0)//空列表没有任务返回空
    {
        fputs("queue is empty\n",stderr);
        return NULL;
    }
    if (task_queue->size==1)//有一个任务的任务队列因为头尾重合特殊处理
    {
        Task *task = task_queue->head->task;//获取任务队列中唯一一个任务的地址
        //释放空间此时头和尾的节点的next和pre都为空，而task不能释放所以只释放俩个头尾节点(因为它们指向了同一个地址所以释放一次头或者是尾)不释放节点组成元素的空间
        free(task_queue->head);
        task_queue->size = 0;
        return task;
    }
    //其他情况
    Task* task  = task_queue->head->task;
    TaskQueueNode *head = task_queue->head;
    task_queue->head = head->next;
    head->next->pre = NULL;
    free(head); 
    task_queue->size--;
    return task;
}
/**
 * @brief 注销队列
 * 
 * @param queue 
 */
void destroy_task_queue(TaskQueue*queue){
    while (queue->size!=0)//一个个的删除
    {
        free(dequeue(queue));//因为出队列不会删除任务变量的地址所以在调用外面加上一层释放函数
    }
}
#endif