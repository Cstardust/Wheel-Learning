#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <cstdint>
#include <queue>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cassert>
#include <functional>
#include <iostream>
#include <thread>
#include <unordered_map>

using std::cout;
using std::endl;

class Task
{
public: 
    virtual void run() = 0;
};




class Thread
{
public:
    // using ThreadFunc = std::function<void(std::shared_ptr<Task> task)>;
    using ThreadFunc = std::function<void(void)>;
    Thread(ThreadFunc f);
    ~Thread();
    void start();    
private:
    ThreadFunc threadFunc_;
};



class ThreadPool
{
public:
    ThreadPool(uint32_t max_task_num = 10 , uint32_t max_thread_num = 4);
    ~ThreadPool();
    void start();
    bool submit(std::shared_ptr<Task>); //  user submit task into the taskQueue
    
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
private:
    void threadFunc();                  //  返回类型 ? 
private:
    const uint32_t MAX_TASK_NUM;        //  the max number of task in task queue
    const uint32_t MAX_THREAD_NUM;      //  the max number of thread in thread queue (ThreadPool / threadLists_)
    //  注意要在ThreadPool的deconstruct中 释放Thread对象
    std::vector<std::unique_ptr<Thread>> threadLists_;  //  not need to be thread safe. 使用 unique_ptr. ThreadPool析构时自动释放Thread对象.
    std::queue<std::shared_ptr<Task>> taskLists_;       //  should be thread safe. shared_ptr to prevent from user passing a task being deconstructed
    std::atomic_uint taskN_{0};             //  当前taskQueue中有几个task. 其实就是taskQueue.size() 不过taskQueue.size()需要保证当前一定获得了taskQueue的lock 
    std::atomic_int threadN_{0};             //  当前threadPool中有几个thread
    
    std::mutex taskMtx_;                    //  mutex prevent task queue , make it to be thread safe 
    std::condition_variable taskCond_;      //  producer 和 consumer用一个cond
    std::atomic_bool isRunning_;

    std::condition_variable exitCond_;
};

//  对于 task queue , 可能有多个thread同操作. 
    //  main thread 可能向其中添加task
    //  work threads 可能从其中取走task
//  对于 thread queue , 只有main thread可能会操作.(扩容)

#endif