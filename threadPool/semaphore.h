#include <thread>
#include <mutex>
#include <condition_variable>
#include<cassert>
#include <algorithm>

class Semaphore
{
public:
    Semaphore(int limit)
        : limit_(limit)
    {
        assert(limit_ >= 1);
        sem_ = limit_;
    }

    // ++ max to limit_ , never block
    void V()
    {
        std::unique_lock<std::mutex> lock(semMtx_);
        sem_ = std::max(sem_+1,limit_);       //  sem_++
        semCond_.notify_all();                //  唤醒P操作
    }

    // block and --
    void P()
    {
        std::unique_lock<std::mutex> lock(semMtx_);
        //  cond wait (block)
        while(!(sem_ > 0))
        {
            semCond_.wait(lock);
        }        
        assert(sem_ > 0);
        //  action
        --sem_;
        //  不必notify.
    }

private:
    int limit_;
    int sem_;
    std::mutex semMtx_;
    std::condition_variable semCond_;
};
