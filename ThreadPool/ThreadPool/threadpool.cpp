#include"threadpool.h"
#include<thread>
#include<functional>
#include<iostream>


const int TASK_MAX_THRESHHOLD = 4;

//线程池构造
ThreadPool::ThreadPool() 
	:initThreadSize_(0)
	,taskSize_(0)
	,taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
	,poolMode_(PoolMode::MODE_FIXED)
{}

//线程析构
ThreadPool::~ThreadPool()
{}

//设置线程池工作模式
void ThreadPool::setMode(PoolMode mode)
{
	poolMode_ = mode;
}

//设置task任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	taskQueMaxThreshHold_ = threshhold;
}

//给线程池提交任务,生产任务
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	// 获取锁
	std::unique_lock<std::mutex>lock(taskQueMtx_);

	//线程通信 等待任务队列有空余
	//while(taskQue_.size() == taskQueMaxThreshHold_)
	//{
	//	notFull_.wait(lock);  //当前线程进入等待状态
	//}
	//用户提交任务，最长不能阻塞超过1s，否则判断提交任务失败
	if (!notFull_.wait_for(lock, std::chrono::seconds(1),
		[&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))
	{
		//表示notFull_等待1s钟,条件依然没有满足
		std::cerr << "task queue is full submit task fail" << std::endl;
		//不能这样return task->getResult(); 因为执行完task，task就被析构了
		return Result(sp, false);
	}
	
	//如果有空余，任务放入任务队列
	taskQue_.emplace(sp);
	taskSize_++;

	//因为放入任务，任务队列不空,在notEmpty_上通知
	notEmpty_.notify_all();

	//返回任务的Resu对象
	return Result(sp);
}

//开启线程
void ThreadPool::start(int initThreadSize)
{
	//记录初始线程个数
	initThreadSize_ = initThreadSize;

	//创建线程对象
	for (int i = 0; i < initThreadSize_; i++)
	{
		//创建thread线程对象时，把线程函数给到thread对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
		threads_.emplace_back(std::move(ptr));
	}
	//启动所有线程
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start(); //需要执行一个线程函数
	}
}

//定义线程函数,消费任务
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
			//先获取锁
			std::unique_lock<std::mutex>lock(taskQueMtx_);

			std::cout << "tid： " << std::this_thread::get_id()
				<<"尝试获取任务" << std::endl;

			//等待notEmpty条件
			notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0; });

			std::cout << "tid： " << std::this_thread::get_id()
				<< "获取任务成功" << std::endl;

			//从任务队列中取一个任务
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;

			//如果依然有剩余任务，继续通知其他线程执行任务
			if (taskQue_.size() > 0)
			{
				notEmpty_.notify_all();
			}

			//取出一个任务进行通知，通知继续生产
			notFull_.notify_all();
		}
		//出作用域把锁释放掉

		//当前线程负责执行这个任务
		if (task != nullptr)
		{
			//task->run(); //执行任务,把返回值通过isval方法给到Result
			task->exec();
		}
	}

}

//------------线程方法实现------------

//启动线程
void Thread::start()
{
	//创建一个线程来执行线程函数
	std::thread t(func_);   //C++11,来说，线程对象t，和线程函数func_
	t.detach();    //设置分离线程   pthread_detach pthread_t 设置成分离线程
}

Thread::Thread(ThreadFunc func) :func_(func)
{

}

Thread::~Thread()
{

}

//------------Result方法的实现----------

Result::Result(std::shared_ptr<Task> task, bool isValid)
	:isValid_(isValid),task_(task)
{
	task_->setResult(this);
}

void Result::setVal(Any any)
{
	//存储task的返回值
	this->any_ = std::move(any);
	sem_.post();        //已经获取任务返回值,增加信号量资源

}

Any Result::get()
{
	if (!isValid_)
	{
		return "";
	}
	sem_.wait(); //task任务如果没有执行完,这里会阻塞用户线程
	return std::move(any_);
}


//-------------Task方法实现

Task::Task():result_(nullptr)
{
}

void Task::exec()
{
	if (result_ != nullptr)
	{
		result_->setVal(run());  //这里发生多态调用
	}
}

void Task::setResult(Result* res)
{
	result_ = res;
}
