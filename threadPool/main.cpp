#include "threadpool.h"
#include <pthread.h>

class ConcreteTask : public Task
{
public:
    void run() override
    {
        cout<<"hello world!"<<endl;
        // std::this_thread::sleep_for(std::chrono::microseconds(2000000));
        long long cnt = 0;
        for(long long i=0;i<100000000;++i)
        {
            ++cnt;
        }
        cout<<"bye world!"<<endl;
    }
};

int main()
{

    {
        cout<<"========main start======"<<endl;

        ThreadPool threadPool(10,1);
        threadPool.start();     //  thread pool里面的 threads 开始运行. main thread返回回来.
        
        int N = 1;

        while(N--)
        {
            std::shared_ptr<Task> sp(new ConcreteTask());
            threadPool.submit(sp);
        }

        std::this_thread::sleep_for(std::chrono::microseconds(2000000));
    }

    // std::shared_ptr<std::mutex> m(new std::mutex());
    // getchar();
    cout<<"========main end======"<<endl;
    pthread_exit(0);
}