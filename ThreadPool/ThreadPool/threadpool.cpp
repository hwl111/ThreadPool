#include"threadpool.h"
#include<thread>
#include<functional>
#include<iostream>


const int TASK_MAX_THRESHHOLD = 1024;

//�̳߳ع���
ThreadPool::ThreadPool() 
	:initThreadSize_(0)
	,taskSize_(0)
	,taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
	,poolMode_(PoolMode::MODE_FIXED)
{}

//�߳�����
ThreadPool::~ThreadPool()
{}

//�����̳߳ع���ģʽ
void ThreadPool::setMode(PoolMode mode)
{
	poolMode_ = mode;
}

//����task�������������ֵ
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	taskQueMaxThreshHold_ = threshhold;
}

//���̳߳��ύ����,��������
void ThreadPool::subnitTask(std::shared_ptr<Task> sp)
{

}

//�����߳�
void ThreadPool::start(int initThreadSize)
{
	//��¼��ʼ�̸߳���
	initThreadSize_ = initThreadSize;

	//�����̶߳���
	for (int i = 0; i < initThreadSize_; i++)
	{
		//����thread�̶߳���ʱ�����̺߳�������thread����
		threads_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc,this)));
	}
	//���������߳�
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start(); //��Ҫִ��һ���̺߳���
	}
}

//�����̺߳���,��������
void ThreadPool::threadFunc()
{
	std::cout << "begin threadfunc tid" <<std::this_thread::get_id()
		<<std::endl;

	std::cout << "end threadfunc tid"<<std::this_thread::get_id()
		<<std::endl;
}

//------------�̷߳���ʵ��------------

//�����߳�
void Thread::start()
{
	//����һ���߳���ִ���̺߳���
	std::thread t(func_);   //C++11,��˵���̶߳���t�����̺߳���func_
	t.detach();    //���÷����߳�   pthread_detach pthread_t ���óɷ����߳�
}

Thread::Thread(ThreadFunc func) :func_(func)
{

}

Thread::~Thread()
{

}
