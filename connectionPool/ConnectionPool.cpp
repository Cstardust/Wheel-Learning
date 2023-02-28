
#include"ConnectionPool.h"
#include <thread>

//  atomic_int reuse_cnt = 0;
//  atomic_int produce_cnt = 0;

ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool;
	return &pool;
}

ConnectionPool::~ConnectionPool(){
	_cv.notify_all();			//  唤醒producer
	unique_lock<std::mutex> lock(_queueMtx);		//  防止其他thread操作connectionQueue
	while (!_connectionQueue.empty()) {
		Connection * p = _connectionQueue.front();
		delete p;
		_connectionQueue.pop();
	}
}

bool ConnectionPool::loadConfigFile()
{
	FILE* pf = fopen("mysql.ini", "r");
	if (pf == nullptr) {
		cerr << "mysql.ini file is not existed" << endl;;
		return false;
	}
	while (!feof(pf))
	{
		char line[512] = {0};
		if (fgets(line, 512, pf) == nullptr) {
			break;
		}
		// port=13306
		string str(line);
		int idx = str.find('=');
		if (idx == -1) continue;
		// username=root
		int end_idx = str.find('\n',idx);	
		if (end_idx == -1) break;
		string key(str.substr(0, idx));		
		string val(str.substr(idx + 1, end_idx - 1 - (idx + 1) + 1));
		if (key == "ip") {
			_ip = val;
		}
		else if (key == "port") {
			_port = atoi(val.c_str());
		}
		else if (key == "username") {
			_name = val;
		}
		else if (key == "password") {
			_password = val;
		}
		else if (key == "dbname") {
			_dbname = val;
		}
		else if (key == "initSize") {
			_initSize = atoi(val.c_str());
		}
		else if (key == "maxSize") {
			_maxSize = atoi(val.c_str());
		}
		else if (key == "maxIdleTime") {
			_maxIdleTime = atoi(val.c_str());
		}
		else if (key == "maxConnectionTimeout") {
			_connectionTimeout = atoi(val.c_str());
		}
	}
	return true;
}

//  初始化
ConnectionPool::ConnectionPool()
{
	//  加载配置
	if (!loadConfigFile()) {
		throw "Failed to Load Config File";
		// return;		//   throw吧
	}
	//  创建初始数量的连接
	for (int i = 0; i < _initSize; ++i) {
		Connection* p = new Connection();
		p->connect(_ip, _port,_name, _password, _dbname);
		_connectionQueue.push(p);	//  空闲连接加入队列
		p->refreshAliveTime();		//  更新时间
		++_connCnt;
	}

	//  启动一个新的线程，作为连接的生产者。
	std::thread produce(&ConnectionPool::produceConnectionTask, this);
	produce.detach();	
	//  扫描线程
	std::thread scanner(&ConnectionPool::scannConnectionTask, this);
	scanner.detach();	

}

//  生产者-消费者模型 
//  produce Connection。
void ConnectionPool::produceConnectionTask()
{
	while (isRunning_)
	{
		unique_lock<mutex> uni_lck(_queueMtx);
		//  producer : 空了再生产
		while (isRunning_ && !_connectionQueue.empty()) {
			_cv.wait(uni_lck);
			if(!isRunning_)
				break;			
		}
		if(!isRunning_)
			break;
		
		if (_connCnt < _maxSize) {
			Connection* pc = new Connection();
			if (!pc->connect(_ip, _port, _name, _password, _dbname)) {
				cerr << "MySQL connection failed" << endl;
				continue;
			}
			++_connCnt;
			_connectionQueue.push(pc);
			pc->refreshAliveTime();
		}
		//  cout<<"produce_cnt "<<produce_cnt++<<endl;
		_cv.notify_all();
	}
	cerr<<"producer thread exit"<<endl;
}

//  消费者线程
//  给外部提供接口，从连接池中获取一个可用的空闲连接
shared_ptr<Connection> ConnectionPool::getConnection()
{
	//  上锁
	unique_lock<mutex> uni_lck(_queueMtx);
	//  等待
	while (_connectionQueue.empty()) {
		// //  为空、通知生产
		 _cv.notify_all();
		//  等待_connectionTimeout ms
		cv_status st = _cv.wait_for(uni_lck,chrono::milliseconds(_connectionTimeout));
		if (st == cv_status::timeout && _connectionQueue.empty()) {		//  超时且队列为空，则无法再获取新连接
			cerr << "timeout for getting a free Connection!"<<endl;;
			return nullptr;
		}
	}

	//  消费 返回给用户一个shared_ptr<Connection>. 重定义析构器 : connection被放回queue
	shared_ptr<Connection> sp(_connectionQueue.front(), [&](Connection* pcon)->void {
		//  cout<<"reuse_cnt "<<++reuse_cnt << endl;
			lock_guard<mutex> guard(_queueMtx);		//  线程安全
			_connectionQueue.push(pcon);
			pcon->refreshAliveTime();
		});
	_connectionQueue.pop();

	//  队列为空，则通知生产
	if (_connectionQueue.empty()) {
		_cv.notify_all();
	}

	//  返回
	return sp;
}


void ConnectionPool::scannConnectionTask()
{
	while (isRunning_)
	{
		//  定时检查
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));	//  60s
		//  扫描队列 释放资源
		lock_guard<mutex> guard(_queueMtx);
		while (isRunning_ && !(_connectionQueue.empty()) && _connectionQueue.size()>_initSize) {
			Connection* pt = _connectionQueue.front();
			if (pt->getAliveTime() <= _maxIdleTime*1000)	//  getAliveTime单减
				break;
			_connectionQueue.pop();	
			--_connCnt;				//  空闲连接数量--
			delete pt;				//  释放连接
		}
	}
}



/*
* 线程分离
* 线程分离状态：指定该状态，线程主动与主控线程断开关系。线程结束后，其退出状态不由其他线程获取，而
直接自己自动释放。网络、多线程服务器常用。
进程若有该机制，将不会产生僵尸进程。僵尸进程的产生主要由于进程死后，大部分资源被释放，一点残留资
源仍存于系统中，导致内核认为该进程仍存在
*/