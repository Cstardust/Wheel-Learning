#ifndef _CONNECTION_POOL_H_
#define _CONNECTION_POOL_H_

#include"Connection.h"
#include<string>
#include<queue>
#include<mutex>
#include<atomic>
#include<functional>
#include<condition_variable>
#include<memory>

using std::atomic_int;
using std::string;
using std::queue;
using std::mutex;


/*
* 线程池
* 单例模式
*/
class ConnectionPool {
public:
	//  static函数：为了在还没有对象时就可调用
	static ConnectionPool* getConnectionPool();
	//  消费者线程
	//  给外部提供接口，从连接池重获取一个可用的空闲连接 
	//  我们需要重定义shared_ptr的析构器
	shared_ptr<Connection> getConnection();

	ConnectionPool(const ConnectionPool&) = delete;
	ConnectionPool& operator=(const ConnectionPool&) = delete;
	~ConnectionPool();
private:
	//  构造函数私有
	ConnectionPool();		
	//  加载配置文件
	bool loadConfigFile();		
	//  生产者线程
	//  运行在独立的线程中，专门负责生产新连接
	//  设置为成员函数而非全局函数的好处：可以方便的访问连接池中的成员
	void produceConnectionTask();	

	//  定时检测线程
	//  定时检测并释放空闲连接
	void scannConnectionTask();

	//  mysql信息
	string _ip;					//  mysql server ip
	unsigned  _port;			//  mysql server port
	string _name;				//  mysql server 登录用户名
	string _password;			//  mysql server 登录密码
	string _dbname;				//  要访问的数据库名称
	//  配置连接池
	int _initSize;				//  连接池的初始连接量    
	int _maxSize;				//  连接池空闲连接的最大连接量	       1024个  queue里最多只能有1024个空闲连接。
	int _maxIdleTime;			//  连接池中空闲连接的最大空闲时间	   60s  (在queue里待的最长时间）超过则释放。scan线程里用到。本压测代码不会触发。
	int _connectionTimeout;		//  用户从连接池获取连接的超时时间     100ms  超时后则放弃获取连接。在getConnection线程里用到。本压测代码不会被触发。	

	//  连接池自己的成员
	queue<Connection*> _connectionQueue;	//  存储mysql连接的队列
	volatile atomic_int _connCnt;	    	//  连接数目
	condition_variable _cv;					//  条件变量 （生产者消费者模型）
	mutex _queueMtx;						//  锁

	std::atomic_bool isRunning_{true};
	// Semaphore _sem;	
};


#endif 
