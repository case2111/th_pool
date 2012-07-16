#ifndef __THREAD_POOL__
#define __THREAD_POOL__ 

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

//#define thread_debug(message, args...)  printf("Line %4d in %s: --"message"", __LINE__, __FILE__, ##args);
#define thread_error(message, args...)  printf("[ERROR]Line %4d in %s: --"message"\n", __LINE__, __FILE__, ##args);
#define thread_debug(message, args...)

//typedef struct _WORK_PROCESS WORK_PROCESS;
//typedef struct _THREAD_WORK_ARGUMENTS WORK_ARGUMENTS;
typedef struct _THREAD_INFO  THREAD_INFO;
typedef struct _THREAD_POOL  THREAD_POOL;

THREAD_POOL *creat_thread_pool(int min_num, int max_num);
void add_job(THREAD_POOL *pool, void *thread_work, void *args);
void close_thread_pool(THREAD_POOL *pool);
static void *work_thread(void *pthread);
static void *manage_thread(void *pthread);
static int delete_thread(THREAD_POOL *pool);
static int add_thread(THREAD_INFO *new_thread);



struct _WORK_PROCESS
{
    void (*process_job)(void *arg);
};

struct _THREAD_INFO
{
    pthread_t    thread_id;
    int            busy_status;
    pthread_cond_t        thread_cond;
    pthread_mutex_t        thread_lock;
    void (*process_work)(void *);
    void    *job_arg;
};

struct _THREAD_POOL
{
    int min_th_num;
    int cur_th_num;
    int max_th_num;
    pthread_mutex_t pool_lock;
    pthread_t manage_thread_id;
    THREAD_INFO *thread_info;
};

#endif
