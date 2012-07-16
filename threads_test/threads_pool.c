#include "threads_pool.h"
#define TH_STACK_SIZE 5120

#define TH_BUSY      1
#define TH_TEMP_BUSY 2
#define TH_IDLE      3
#define TH_DIED      4

THREAD_POOL *creat_thread_pool(int min_num, int max_num)
{
    THREAD_POOL *tmp_thread_pool;
    tmp_thread_pool = (THREAD_POOL *)malloc(sizeof(THREAD_POOL));
    memset(tmp_thread_pool, 0, sizeof(THREAD_POOL));
    tmp_thread_pool->min_th_num = min_num;
    tmp_thread_pool->max_th_num = max_num;
    tmp_thread_pool->cur_th_num = min_num;
    pthread_mutex_init(&tmp_thread_pool->pool_lock, NULL);
    tmp_thread_pool->thread_info = (THREAD_INFO *)malloc(sizeof(THREAD_INFO)*max_num);
	memset(tmp_thread_pool->thread_info, 0, sizeof(THREAD_INFO));
    return tmp_thread_pool;
}

int init_thread_pool(THREAD_POOL *pool)
{
    int th_count;
    int err;
	pthread_attr_t  oAttr;

    for(th_count = 0;th_count < pool->min_th_num; th_count++)
    {
        pthread_cond_init(&pool->thread_info[th_count].thread_cond, NULL);
        pthread_mutex_init(&pool->thread_info[th_count].thread_lock, NULL);
		pool->thread_info[th_count].busy_status = TH_IDLE;

        pthread_mutex_lock(&pool->thread_info[th_count].thread_lock);
		pthread_attr_init(&oAttr);
		pthread_attr_setdetachstate(&oAttr, PTHREAD_CREATE_DETACHED);
		pthread_attr_setstacksize(&oAttr, TH_STACK_SIZE);
        err = pthread_create(&pool->thread_info[th_count].thread_id, &oAttr, work_thread, &pool->thread_info[th_count]);
		pthread_attr_destroy(&oAttr);
        if(err != 0)
        {
            thread_debug("init_thread_pool: creat work thread failed\n");
            return -1;
        }
        thread_debug("init_thread_pool: creat work thread %x success\n", pool->thread_info[th_count].thread_id);
    }
    /*err = pthread_create(&pool->manage_thread_id, NULL, manage_thread, pool);
    if(err != 0)
    {
        thread_debug("init_thread_pool: create manage thread %d failed\n", pool->manage_thread_id);
        return -1;
    }
    thread_debug("init_thread_pool: create manage thread %x success\n", pool->manage_thread_id);*/
    return 0;
}

void close_thread_pool(THREAD_POOL *pool)
{
    int th_count;
    for(th_count = 0; th_count < pool->cur_th_num; th_count++)
    {
        kill(pool->thread_info[th_count].thread_id, SIGKILL);
        pthread_mutex_destroy(&pool->thread_info[th_count].thread_lock);
        pthread_cond_destroy(&pool->thread_info[th_count].thread_cond);
        thread_debug("close the work thread %x\n", pool->thread_info[th_count].thread_id);
    }
    kill(pool->manage_thread_id, SIGKILL);
    pthread_mutex_destroy(&pool->pool_lock);
    thread_debug("close the manage thread %x\n", pool->manage_thread_id);
    free(pool->thread_info);
}

void add_job(THREAD_POOL *pool, void *thread_work, void *args)
{
    int th_count;
	int add_th_status;
	int count_add_th = 50;
    while(count_add_th--)
	{
		for(th_count = 0; th_count < pool->max_th_num; th_count++)
		{
			if(th_count < pool->min_th_num)
			{
				if(pool->thread_info[th_count].busy_status == TH_IDLE)
				{
					thread_debug("found a idle thread\n");
					break;
				}
			}
			else
			{
				if(pool->thread_info[th_count].busy_status == TH_IDLE)
				{
					thread_debug("found a idle thread\n");
					break;
				}
				if(pool->thread_info[th_count].busy_status != TH_TEMP_BUSY)
				{
					add_th_status = add_thread(&pool->thread_info[th_count]);
					if(add_th_status == 0)
					{
						pool->thread_info[th_count].process_work = thread_work;
						pool->thread_info[th_count].job_arg = args;
						break;
					}
					else
					{
						thread_error("create new thread faile\n");
						return;
					}
					
				}
			}
		}
		if(th_count == pool->max_th_num)
		{
			thread_debug("reach to max thread\n");
			usleep(10000);
			continue;
		}
		usleep(1000);
		for(th_count = 0; th_count < pool->max_th_num; th_count++)
		{
			thread_debug("Begin add job thread status:%d!!!!!!\n", pool->thread_info[th_count].busy_status);
			if(pool->thread_info[th_count].busy_status == TH_IDLE)
			{
				thread_debug("the thread %d idle\n", th_count);
				if(th_count < pool->min_th_num)
				{
					pool->thread_info[th_count].busy_status = TH_BUSY;
				}
				else
				{
					pool->thread_info[th_count].busy_status = TH_TEMP_BUSY;
				}
				pool->thread_info[th_count].process_work = thread_work;
				pool->thread_info[th_count].job_arg = args;
				thread_debug("the thread %d start working\n", th_count);
				pthread_mutex_lock(&pool->thread_info[th_count].thread_lock);
				pthread_cond_signal(&pool->thread_info[th_count].thread_cond);
				pthread_mutex_unlock(&pool->thread_info[th_count].thread_lock);
				return;
			}
		}
	}
    thread_error("create new thread faile\n");
}

static int add_thread(THREAD_INFO *new_thread)
{
    int err;
	pthread_attr_t  oAttr;
    //THREAD_INFO *new_thread;
    /*if(pool->cur_th_num >= pool->max_th_num)
    {
        thread_debug("thread numbers reach to max\n");
        return 1;
    }*/
   // new_thread = &pool->thread_info[pool->cur_th_num];
    new_thread->busy_status = TH_IDLE;
    pthread_cond_init(&new_thread->thread_cond, NULL);
    pthread_mutex_init(&new_thread->thread_lock, NULL);
	//pthread_mutex_lock(&pool->pool_lock);
    //pool->cur_th_num++;
	//pthread_mutex_unlock(&pool->pool_lock);
    pthread_mutex_lock(&new_thread->thread_lock);
	pthread_attr_init(&oAttr);
	pthread_attr_setdetachstate(&oAttr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&oAttr, TH_STACK_SIZE);
    err = pthread_create(&new_thread->thread_id, &oAttr, work_thread, new_thread);
	pthread_attr_destroy(&oAttr);
    if(err != 0)
    {
        return -1;
    }
    thread_debug("create new thread : %x success\n", new_thread->thread_id);
    return 0;
}
static void thread_status(THREAD_POOL *pool)
{
	int count;
	for(count = 0; count < pool->cur_th_num; count++)
	{
		thread_debug("the thread:%x working\n",  pool->thread_info[count].thread_id);
	}
}
static int delete_thread(THREAD_POOL *pool)
{

    if(pool->cur_th_num <= pool->min_th_num)
    {
        return;
    }
    if(pool->thread_info[pool->cur_th_num - 1].busy_status)
    {
        return;
    }
    pthread_mutex_lock(&pool->pool_lock);
    if(pool->cur_th_num > pool->min_th_num)
    {
        kill(pool->thread_info[pool->cur_th_num - 1].thread_id, SIGKILL);
        pthread_mutex_destroy(&pool->thread_info[pool->cur_th_num - 1].thread_lock);
        pthread_cond_destroy(&pool->thread_info[pool->cur_th_num - 1].thread_cond);
        pool->cur_th_num--;
    }
    pthread_mutex_unlock(&pool->pool_lock);
    return 0;
}

static void *work_thread(void *pthread)
{
    //pthread_t cur_id;
    //int        th_count;
    THREAD_INFO *thread_info;
    thread_info = (THREAD_INFO *)pthread;
    int firsRun = 1;
    //cur_id = pthread_self();
    
    while(1)
    {
        if(!firsRun)
        {
            pthread_mutex_lock(&thread_info->thread_lock);
        }
        else //if first run,thread_lock already locked by creat function. do not need lock again
        {
            firsRun = 0;
        }
        
        //pthread_mutex_lock(&thread_info->thread_lock);
        //thread_debug("***************balock working thread id %x:::%x\n", cur_id, thread_info->thread_id);
        pthread_cond_wait(&thread_info->thread_cond, &thread_info->thread_lock);
        //thread_debug("###############beginning workingthread id %x:::%x\n", cur_id, thread_info->thread_id);
        pthread_mutex_unlock(&thread_info->thread_lock);

        void (*work)(void *);
        work = thread_info->process_work;
        void *args = thread_info->job_arg;
        
        
        work((void *)args);
        
        
        //pthread_mutex_lock(&thread_info->thread_lock);
        //thread_debug("_________________beginning workingthread id %x:::%x\n", cur_id, thread_info->thread_id);
		if(thread_info->busy_status == TH_BUSY)
		{
        	thread_info->busy_status = TH_IDLE;
		}
		if(thread_info->busy_status == TH_TEMP_BUSY)
		{
			thread_info->busy_status = TH_DIED;
			return;
		}
		//pthread_mutex_unlock(&thread_info->thread_lock);
    }
}

static void *manage_thread(void *pthread)
{
    THREAD_POOL *pool = (THREAD_POOL *)pthread;
    while(1)
    {
        delete_thread(pool);
        usleep(10000);
    }
}
/*static count = 1;
static void test_work(void *args)
{
    pthread_mutex_t work_lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&work_lock);
    WORK_ARGUMENTS *pagrs;
    thread_debug("%d######test_work working\n", count++);
    pthread_mutex_unlock(&work_lock);
    pthread_mutex_destroy(&work_lock);
}
main(void)
{
    WORK_ARGUMENTS *args;
    void (*job)(void *);
    char *agrs = "";
    THREAD_POOL *pool = creat_thread_pool(5, 100);
    init_thread_pool(pool);
    job = test_work;
    int i = 1000;
    while(i--)
    {
        thread_debug("will add job\n");
        add_job(pool, job, args);
        add_job(pool, job, args);
        add_job(pool, job, args);
        add_job(pool, job, args);
        add_job(pool, job, args);
        add_job(pool, job, args);
        add_job(pool, job, args);
        add_job(pool, job, args);
        add_job(pool, job, args);
        add_job(pool, job, args);
        add_job(pool, job, args);
        thread_debug("add job end\n");
        //sleep(2);
    }
    usleep(80000);
    close_thread_pool(pool);
}*/
