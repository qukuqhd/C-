#include <stdio.h>
#include "thread_pool.h"
void* taskFunc(void* arg)
{
    int num = *(int*)arg;
    printf("thread %ld is working, number = %d\n",
        pthread_self(), num);
    sleep(1);
    return NULL;
}
int main(int, char**){
    ThreadPool* pool = init_thread_pool(3,10);
    for (int i = 0; i < 100; ++i)
    {
        int* num = (int*)malloc(sizeof(int));
        *num = i + 100;
        thread_pool_add_task(pool, taskFunc, num);
    }
    sleep(30);
    destroy_thread_pool(pool);
    return 0;   
}
