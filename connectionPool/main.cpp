#include "Connection.h"
#include "ConnectionPool.h"
#include <string.h>
#include <thread>

void work()
{
	const int maxn = 10000;
	for (int i = 0; i < maxn / 4; ++i)
	{
		Connection conn;
		char sql[1024] = {0};
		sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "female");
		conn.connect("127.0.0.1", 13306, "root", "C361456shc", "chat");
		conn.update(sql);
	}
}

int main()
{
	//  ConnectionPool* p = ConnectionPool::getConnectionPool();
	/*
	//  添加测试MySQL第三方库接口
	Connection conn;
	char sql[1024];
	sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
		"zhang san", 20, "female");
	conn.connect("127.0.0.1", 13306, "root", "C361456shc","chat");
	conn.update(sql);
	*/
#if 0
#if 0
	clock_t bg = clock();
	for (int i = 0; i < 10000; ++i)
	{
		Connection conn;
		char sql[1024]={0};
		sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
			"zhang san", 20, "female");
		conn.connect("127.0.0.1", 13306, "root", "C361456shc", "chat");
		conn.update(sql);
	}
	clock_t ed = clock();
	cout << (ed - bg) << "ms" << endl;

#else

	clock_t bg = clock();
	//  ConnectionPool 开启 producer thread , scan thread
	//  main thread 就是 consume thread
	ConnectionPool *p = ConnectionPool::getConnectionPool();
	for (int i = 0; i < 10000; ++i)
	{
		char sql[1024] = {0};
		sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "female");
		shared_ptr<Connection> sp = p->getConnection();
		sp->update(sql);
		//  deleter : Connection放回queue
	}
	clock_t ed = clock();
	cout << (ed - bg) << "ms" << endl;
#endif

#else
#if 0
	 Connection conn;
	 conn.connect("127.0.0.1", 13306, "root", "C361456shc", "chat");
	clock_t bg = clock();
	thread t1(work);
	thread t2(work);
	thread t3(work);
	thread t4(work);
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	clock_t ed = clock();
	std::cout << (ed - bg) << "ms" << std::endl;
#else
	clock_t bg = clock();

	ConnectionPool *p = ConnectionPool::getConnectionPool();
	const int maxn = 10000;
	std::thread t1([&]()
			  {
		for (int i = 0; i < maxn / 4; ++i)
		{
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "female");
			shared_ptr<Connection> sp = p->getConnection();
			sp->update(sql);
		} });

	std::thread t2([&]()
			  {
		for (int i = 0; i < maxn / 4; ++i)
		{
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "female");
			shared_ptr<Connection> sp = p->getConnection();
			sp->update(sql);
		} });

	std::thread t3([&]()
			  {
		for (int i = 0; i < maxn / 4; ++i)
		{
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "female");
			shared_ptr<Connection> sp = p->getConnection();
			sp->update(sql);
		} });

	std::thread t4([&]()
			  {
		for (int i = 0; i < maxn / 4; ++i)
		{
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "female");
			shared_ptr<Connection> sp = p->getConnection();
			sp->update(sql);
		} });

	t1.join();
	t2.join();
	t3.join();
	t4.join();

	clock_t ed = clock();
	cout << (ed - bg) << "ms" << endl;
#endif

#endif
}