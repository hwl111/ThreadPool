#include"threadpool.h"
#include<thread>
#include<functional>
#include<iostream>


const int TASK_MAX_THRESHHOLD = INT32_MAX;
const int THREAD_MAX_THRESHHOLD = 100;
const int THREAD_MAX_IDLE_TIME = 10;   //��λ ��
//�̳߳ع���
ThreadPool::ThreadPool() 
	:initThreadSize_(0)
	,taskSize_(0)
	,curThreadSize_(0)
	, threadSizeThreshHold_(THREAD_MAX_THRESHHOLD)
	,taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
	,poolMode_(PoolMode::MODE_FIXED)
	,isPoolRunning_(false)
	,idleThreadSize_(0)
{}

//�߳�����
ThreadPool::~ThreadPool()
{}

//�����̳߳ع���ģʽ
void ThreadPool::setMode(PoolMode mode)
{
	if (checkRunningState())
		return;
	poolMode_ = mode;
}

void ThreadPool::setThreadSizeThreshHold(int threshhold)
{
	if (checkRunningState())
		return;
	if (poolMode_ == PoolMode::MODE_CACHED)
	{
		threadSizeThreshHold_ = threshhold;
	}
}

//����task�������������ֵ
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	if (checkRunningState())
		return;
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

	//cachedģʽ��Ҫ�������������Ϳ����̵߳������ж��Ƿ���Ҫ�����µ��߳�
	if (poolMode_ == PoolMode::MODE_CACHED
		&& taskSize_ > idleThreadSize_
		&& curThreadSize_ < threadSizeThreshHold_)
	{
		std::cout << "create new thread...." << std::endl;

		//�������߳�
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		//threads_.emplace_back(std::move(ptr));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));

		//�����߳�
		threads_[threadId]->start();
		//�޸��̸߳�����ر���
		curThreadSize_++;
		idleThreadSize_++;
	}

	//���������Resu����
	return Result(sp);
}

//�����߳�
void ThreadPool::start(int initThreadSize)
{
	//�����̳߳ص�����״̬
	isPoolRunning_ = true;

	//��¼��ʼ�̸߳���
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

	//�����̶߳���
	for (int i = 0; i < initThreadSize_; i++)
	{
		//����thread�̶߳���ʱ�����̺߳�������thread����
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		//threads_.emplace_back(std::move(ptr));

		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
	}
	//���������߳�
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start(); //��Ҫִ��һ���̺߳���
		idleThreadSize_++;    //��¼��ʼ�����߳�����
	}
}

//�����̺߳���,��������
void ThreadPool::threadFunc(int threadid)
{
	auto lastTime = std::chrono::high_resolution_clock().now();

	for (;;)
	{
		std::shared_ptr<Task> task;
		{
			//�Ȼ�ȡ��
			std::unique_lock<std::mutex>lock(taskQueMtx_);

			std::cout << "tid�� " << std::this_thread::get_id()
				<<"���Ի�ȡ����" << std::endl;

			//cachedģʽ�£������Ѿ������˺ܶ��̣߳������߳�Ҫ����
			//��ǰʱ��--��һ���߳�ִ��ʱ�� > 60s
			if (poolMode_ == PoolMode::MODE_CACHED)
			{
				// ÿһ���ӷ���һ��
				while (taskQue_.size() == 0)
				{
					//����������ʱ����
					if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds> (now - lastTime);
						if (dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ > initThreadSize_)
						{
							//��ʼ���յ�ǰ�߳�
							//��¼�߳�������ر����޸�
							//���̴߳��߳��б�����ɾ��
							//threadid => thread����=> ɾ��
							threads_.erase(threadid);
							curThreadSize_--;
							idleThreadSize_--;
							std::cout << "threadid:" << std::this_thread::get_id() << " exit����" << std::endl;
							return;
						}
					
					}
				}
			}
			else
			{
				//�ȴ�notEmpty����
				notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0; });
			}
			idleThreadSize_--;   

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
		idleThreadSize_++;
		//�����̵߳�����ִ�е�ʱ��
		lastTime = std::chrono::high_resolution_clock().now();
	}

}

bool ThreadPool::checkRunningState() const
{

	return isPoolRunning_;
}

//------------�̷߳���ʵ��------------

int Thread::generateId_ = 0;

//�����߳�
void Thread::start()
{
	//����һ���߳���ִ���̺߳���
	std::thread t(func_, threadId_);   //C++11,��˵���̶߳���t�����̺߳���func_
	t.detach();    //���÷����߳�   pthread_detach pthread_t ���óɷ����߳�
}

Thread::Thread(ThreadFunc func) 
	:func_(func)
	,threadId_(generateId_++)
{}

Thread::~Thread() {}

int Thread::getId() const
{
	return threadId_;

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
