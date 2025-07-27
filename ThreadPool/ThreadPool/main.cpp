#include<iostream>
#include<chrono>
#include<thread>

#include"threadpool.h"

/*
��Щ����ϣ����ȡ�̵߳ķ���ֵ
eg.
1 + 2....+30000�ĺ�
thread1 1�ӵ�1��
thread2 10001�ӵ�2��
thread3 20001�ӵ�3��

main thread:��ÿ���̷߳���������䣬���ҵȴ��������귵�ؽ��
�ϲ����ս��
*/


class MyTask : public Task
{
public:
	MyTask(int begin, int end)
		:begin_(begin),end_(end)
	{}

	//������run���Է����������͵�ֵ
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

	//������������result����
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