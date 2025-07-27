#include"threadpool.h"
#include<thread>
#include<functional>
#include<iostream>


const int TASK_MAX_THRESHHOLD = 4;

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
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	// ��ȡ��
	std::unique_lock<std::mutex>lock(taskQueMtx_);

	//�߳�ͨ�� �ȴ���������п���
	//while(taskQue_.size() == taskQueMaxThreshHold_)
	//{
	//	notFull_.wait(lock);  //��ǰ�߳̽���ȴ�״̬
	//}
	//�û��ύ�����������������1s�������ж��ύ����ʧ��
	if (!notFull_.wait_for(lock, std::chrono::seconds(1),
		[&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))
	{
		//��ʾnotFull_�ȴ�1s��,������Ȼû������
		std::cerr << "task queue is full submit task fail" << std::endl;
		//��������return task->getResult(); ��Ϊִ����task��task�ͱ�������
		return Result(sp, false);
	}
	
	//����п��࣬��������������
	taskQue_.emplace(sp);
	taskSize_++;

	//��Ϊ��������������в���,��notEmpty_��֪ͨ
	notEmpty_.notify_all();

	//���������Resu����
	return Result(sp);
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
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
		threads_.emplace_back(std::move(ptr));
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
	/*
	std::cout << "begin threadfunc tid" <<std::this_thread::get_id()
		<<std::endl;

	std::cout << "end threadfunc tid"<<std::this_thread::get_id()
		<<std::endl;
	*/
	for (;;)
	{
		std::shared_ptr<Task> task;
		{
			//�Ȼ�ȡ��
			std::unique_lock<std::mutex>lock(taskQueMtx_);

			std::cout << "tid�� " << std::this_thread::get_id()
				<<"���Ի�ȡ����" << std::endl;

			//�ȴ�notEmpty����
			notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0; });

			std::cout << "tid�� " << std::this_thread::get_id()
				<< "��ȡ����ɹ�" << std::endl;

			//�����������ȡһ������
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;

			//�����Ȼ��ʣ�����񣬼���֪ͨ�����߳�ִ������
			if (taskQue_.size() > 0)
			{
				notEmpty_.notify_all();
			}

			//ȡ��һ���������֪ͨ��֪ͨ��������
			notFull_.notify_all();
		}
		//������������ͷŵ�

		//��ǰ�̸߳���ִ���������
		if (task != nullptr)
		{
			//task->run(); //ִ������,�ѷ���ֵͨ��isval��������Result
			task->exec();
		}
	}

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

//------------Result������ʵ��----------

Result::Result(std::shared_ptr<Task> task, bool isValid)
	:isValid_(isValid),task_(task)
{
	task_->setResult(this);
}

void Result::setVal(Any any)
{
	//�洢task�ķ���ֵ
	this->any_ = std::move(any);
	sem_.post();        //�Ѿ���ȡ���񷵻�ֵ,�����ź�����Դ

}

Any Result::get()
{
	if (!isValid_)
	{
		return "";
	}
	sem_.wait(); //task�������û��ִ����,����������û��߳�
	return std::move(any_);
}


//-------------Task����ʵ��

Task::Task():result_(nullptr)
{
}

void Task::exec()
{
	if (result_ != nullptr)
	{
		result_->setVal(run());  //���﷢����̬����
	}
}

void Task::setResult(Result* res)
{
	result_ = res;
}
