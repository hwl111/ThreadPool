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
using ULong = unsigned long long;

class MyTask : public Task
{
public:
	MyTask(int begin, int end)
		:begin_(begin),end_(end)
	{}

	//Java Python Object是所有其他类型的基类
	//C++17 Any类型--->可以接收任意的其他类型
	Any run() 
	{
		
		std::cout << "begin threadfunc tid: " << std::this_thread::get_id()
			<< std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(3));
		
		ULong sum = 0;
		for (ULong i = begin_; i <= end_; i++)
			sum += i;

		std::cout << "end threadfunc tid: " << std::this_thread::get_id()
			<< std::endl;


		return sum;
	}
private:
	int begin_;
	int end_;
};


int main()
{
	ThreadPool pool;

	//用户自己设置线程池的工作模式
	pool.setMode(PoolMode::MODE_CACHED);
	//启动线程池
	pool.start(4);

	Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 10000000));
	Result res2 = pool.submitTask(std::make_shared<MyTask>(10000001, 20000000));
	Result res3 = pool.submitTask(std::make_shared<MyTask>(20000001, 30000000));
	Result res4 = pool.submitTask(std::make_shared<MyTask>(1, 10000000));
	Result res5 = pool.submitTask(std::make_shared<MyTask>(10000001, 20000000));
	Result res6 = pool.submitTask(std::make_shared<MyTask>(20000001, 30000000));


	ULong sum1 = res1.get().cast_<ULong>();
	ULong sum2 = res2.get().cast_<ULong>();
	ULong sum3 = res3.get().cast_<ULong>();

	//Master - Salve线程模型
	//Master线程用来分解任务,然后给各个Salve线程分配任务
	//等待各个Salve线程完成任务，返回结果
	//Master线程合并各个任务结果，输出
	std::cout << (sum1 + sum2 + sum3) << std::endl;


	ULong sum = 0;
	for (ULong i = 1; i <= 30000000; i++)
		sum += i;
	std::cout << sum << std::endl;

   //get返回一个Any类型,转换成具体类型

//	pool.submitTask(std::make_shared<MyTask>());
//	pool.submitTask(std::make_shared<MyTask>());
//	pool.submitTask(std::make_shared<MyTask>());
//	pool.submitTask(std::make_shared<MyTask>());
//	pool.submitTask(std::make_shared<MyTask>());
//	pool.submitTask(std::make_shared<MyTask>());
//	pool.submitTask(std::make_shared<MyTask>());
//	pool.submitTask(std::make_shared<MyTask>());
//	pool.submitTask(std::make_shared<MyTask>());
//	pool.submitTask(std::make_shared<MyTask>());
//	pool.submitTask(std::make_shared<MyTask>());

	getchar();
}