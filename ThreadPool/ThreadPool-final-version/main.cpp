#include<iostream>
#include<thread>
#include<future>
#include<functional>
#include"threadpool.h"

#ifdef _WIN32
#include <windows.h>
#endif

/*
submitTask用可变参模板编程

c++11 线程库 thread packaged_task(function函数对象) async

使用future代替Result
*/



int sum1(int a, int b)
{
	return a + b;
}

int sum2(int a, int b, int c)
{
	return a + b + c;
}

int main()
{
#ifdef _WIN32
	// 设置控制台编码为UTF-8，解决中文乱码问题
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif

	ThreadPool pool;
	pool.setMode(PoolMode::MODE_CACHED);
	pool.start(2);

	std::future<int>r1 = pool.submitTask(sum1, 10, 30);
	std::future<int>r2 = pool.submitTask([](int begin, int end)->int {
		int sum = 0;
		for (int i = begin; i <= end; i++)
			sum += i;
		return sum;
		},1,100);

	std::cout << r1.get() << std::endl;
	std::cout << r2.get() << std::endl;
}