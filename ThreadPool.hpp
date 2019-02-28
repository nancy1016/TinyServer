#ifndef __THREADPOOL_HPP__
#define __THREADPOOL_HPP__

#include<iostream>
#include<queue>
#include<pthread.h>
typedef void(*handler_t)(int);
class Task
{
    private:
        int sock;
        handler_t handler;
    public:
        Task(int _sock,handler_t _handler)
            :sock(_sock)
             ,handler(_handler)
    {}
        void Run()//处理任务
        {
            handler(sock);
        }
        ~Task()
        {}
};
class ThreadPool
{
private:
    int num;
    std::queue<Task> task_queue;
    int idle_num;//处于等待中的线程个数
    pthread_mutex_t lock;
    pthread_cond_t cond;
public:
    ThreadPool(int _num)
        :num(_num)
         ,idle_num(0)
    {
        pthread_mutex_init(&lock,NULL);
        pthread_cond_init(&cond,NULL);
    }
    void InitThreadPool()
    {
        pthread_t tid;
        for(auto i=0;i<num;i++)
        {
            pthread_create(&tid,NULL,ThreadRoutine,(void*)this);
        }
    }
    bool IsTaskQueueEmpty()
    {
        return task_queue.size()==0?true:false;
    }
    void LockQueue()
    {
        pthread_mutex_lock(&lock);
    }
    void UnlockQueue()
    {
        pthread_mutex_unlock(&lock);
    }
    void Idle()
    {
        idle_num++;
        pthread_cond_wait(&cond,&lock);
        idle_num--;
    }
    //从任务队列中获取任务
    Task PopTask()
    {
        Task t=task_queue.front();
        task_queue.pop();
        return t;
    }
    void Wakeup()
    {
        pthread_cond_signal(&cond);
    }
    void PushTask(Task&t)
    {
        LockQueue();
        task_queue.push(t);
        UnlockQueue();
        Wakeup();
    }
    static void *ThreadRoutine(void*arg)
    {
        pthread_detach(pthread_self());
        ThreadPool *tp=(ThreadPool*)arg;
        for(;;)
        {
            tp->LockQueue();
            while(tp->IsTaskQueueEmpty())
            {
                tp->Idle();
            }
            Task t=tp->PopTask();
            tp->UnlockQueue();
            std::cout<<"task is handler by: "<<pthread_self()<<std::endl;
            t.Run();
        }
    }
    ~ThreadPool()
    {
        pthread_mutex_destroy(&lock);
        pthread_cond_destroy(&cond);
    }



};






#endif
