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
using ULong = unsigned long long;

class MyTask : public Task
{
public:
	MyTask(int begin, int end)
		:begin_(begin),end_(end)
	{}

	//Java Python Object�������������͵Ļ���
	//C++17 Any����--->���Խ����������������
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

	//�û��Լ������̳߳صĹ���ģʽ
	pool.setMode(PoolMode::MODE_CACHED);
	//�����̳߳�
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

	//Master - Salve�߳�ģ��
	//Master�߳������ֽ�����,Ȼ�������Salve�̷߳�������
	//�ȴ�����Salve�߳�������񣬷��ؽ��
	//Master�̺߳ϲ����������������
	std::cout << (sum1 + sum2 + sum3) << std::endl;


	ULong sum = 0;
	for (ULong i = 1; i <= 30000000; i++)
		sum += i;
	std::cout << sum << std::endl;

   //get����һ��Any����,ת���ɾ�������

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