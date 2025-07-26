#include<iostream>
#include<chrono>
#include<thread>

#include"threadpool.h"

int main()
{
	ThreadPool pool;
	pool.start(10);

	std::this_thread::sleep_for(std::chrono::seconds(5));
}