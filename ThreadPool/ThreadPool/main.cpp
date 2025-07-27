#include<iostream>
#include<chrono>
#include<thread>

#include"threadpool.h"

/*
有些场景希望获取线程的返回值
eg.
1 + 2....+30000的和
thread1 1加到1万
thread2 10001加到2万
thread3 20001加到3万

main thread:给每个线程分配计算区间，并且等待他们算完返回结果
合并最终结果
*/


class MyTask : public Task
{
public:
	MyTask(int begin, int end)
		:begin_(begin),end_(end)
	{}

	//如何设计run可以返回任意类型的值
	void run()
	{
		
		std::cout << "begin threadfunc tid: " << std::this_thread::get_id()
			<< std::endl;
		//std::this_thread::sleep_for(std::chrono::seconds(5));
		std::cout << "end threadfunc tid: " << std::this_thread::get_id()
			<< std::endl;
		
		int sum = 0;
		for (int i = begin_; i < end_; i++)
			sum += i;

		return sum;
	}
private:
	int begin_;
	int end_;
};


int main()
{
	ThreadPool pool;
	pool.start(4);

	//如何设置这里的result机制
	Result res = pool.submitTask(std::make_shared<MyTask>());

	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());

	getchar();
}