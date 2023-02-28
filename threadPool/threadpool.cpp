#include "threadpool.h"

ThreadPool::ThreadPool(uint32_t max_task_num /*= 10*/, uint32_t max_thread_num /*= 4*/) : MAX_TASK_NUM(max_task_num),
                                                                                          MAX_THREAD_NUM(max_thread_num),
                                                                                          isRunning_(true)
{
    cout << "construct ThreadPool" << endl;
}

//  thread析构
//  确保其他thread都安全退出
ThreadPool::~ThreadPool()
{
    isRunning_ = false;

    taskCond_.notify_all();
    std::unique_lock<std::mutex> lock(taskMtx_);
    while(!(threadN_ == 0))
    {
        exitCond_.wait(lock);
    }  
    cout << "deconstruct ~ThreadPool" << endl;
}

//  fixed
void ThreadPool::start()
{
    for (uint32_t i = 0; i < MAX_THREAD_NUM; ++i)
    {
        //  创建Thread时 , 将thread 要运行的fun 传递给Thread
        std::unique_ptr<Thread> up = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
        threadLists_.push_back(std::move(up));
        ++threadN_;
    }

    for (auto &t : threadLists_)
    {
        t->start();
    }
}


bool ThreadPool::submit(std::shared_ptr<Task> sp)
{
    // cout << "main start thread begin submit task" << endl;
    std::unique_lock<std::mutex> lock(taskMtx_);
    //  cond wait
    // while(!(taskN_ < MAX_TASK_NUM))
    // {
    //     taskCond_.wait(lock);           //  生产者使用条件变量 代表 : 我希望队列不满
    // }
    // taskCond_.wait(lock,[&]()->bool {return taskN_ < MAX_TASK_NUM; });   //  wait 直到 return taskN_ < MAX_TASK_NUM;
    if (!taskCond_.wait_for(lock,
                            std::chrono::seconds(1),
                            [&]() -> bool
                            { return taskN_ < MAX_TASK_NUM; }))
    {
        //  timeout
        cout << "taskQueue is full , failed to submit the task , timeout!" << endl;
        return false;
    }
    assert(taskN_ < MAX_TASK_NUM);

    //  action
    taskLists_.push(sp);
    ++taskN_;
    
    //  cond notify
    taskCond_.notify_all(); //  全部唤醒 防止生产者唤醒生产者 消费者唤醒消费者 导致极端情况下(队列满/队列空)所有人都在等待.

    // cout << "main start thread end submit task" << endl;

    return true;
}

void ThreadPool::threadFunc()
{
    cout << " thread " << std::this_thread::get_id() << " begin and running threadFunc" << endl;
    while (isRunning_)    //  worker thread at 2 ok
    {
        std::shared_ptr<Task> sp;
        {
            std::unique_lock<std::mutex> lock(taskMtx_);
            //  cond wait
            while (isRunning_ && !(!taskLists_.empty())) //  consumer使用cond 代表 ：我希望队列不空
            {
                taskCond_.wait(lock);
                // if(!isRunning_)             //  worker thread at 1 ok
                // {
                //     cout<<std::this_thread::get_id()<<" not running"<<endl;
                //     break;
                // }
            }
            if(!isRunning_)
            {
                break;
            }
            assert(!taskLists_.empty());
            


            //  action
            sp = taskLists_.front();
            taskLists_.pop();
            --taskN_;
            //  notify
            taskCond_.notify_all(); //  //  全部唤醒 防止生产者唤醒生产者 消费者唤醒消费者 导致极端情况下(队列满/队列空)所有人都在等待.
        }        
        //  bug1 注意 : unique lock 应当在执行 task 之前就该释放. 
            //  lock 只是 用来保护 taskQueue的. 如果不释放的话 , 该thread执行task的过程中 , 所有thread都不能使用taskQueue。都得等待该thread执行task后释放锁.
            //  现象 : thread一个一个执行task
        //  execute task
        if(sp)
        {
            sp->run();
        }
        cout << " thread " << std::this_thread::get_id() << " end running threadFunc" << endl;
    }

    if(!isRunning_)
    {
        cout<<std::this_thread::get_id()<<" will exit"<<endl;
        --threadN_;
        cout<<threadN_.load()<<endl;
        exitCond_.notify_one();
        return ;
    }
}

////////////// Thread /////////////


Thread::Thread(ThreadFunc f) : threadFunc_(f)
{
    cout << "construct Thread" << endl;
}

Thread::~Thread()
{
    cout << "deconstruct ~Thread" << endl;
}

void Thread::start()
{
    // cout << "main thread " << std::this_thread::get_id() << " Thread::start() : create and start a thread " << endl;
    std::thread t(threadFunc_);
    t.detach(); //  线程分离(感觉作用就是延长thread对象的生命周期 使其生命周期和当前线程以及当前线程的函数无关) . 如果不线程分离的话, 那么本函数结束后, thread t 还没运行完 就被销毁了. : terminate called without an active exception
    // cout << "main thread " << std::this_thread::get_id() << " detached and leave Thread::start()" << endl;
}

