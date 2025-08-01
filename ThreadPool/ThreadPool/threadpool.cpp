#include"threadpool.h"
#include<thread>
#include<functional>
#include<iostream>


const int TASK_MAX_THRESHHOLD = INT32_MAX;
const int THREAD_MAX_THRESHHOLD = 100;
const int THREAD_MAX_IDLE_TIME = 10;   //单位 秒
//线程池构造
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

//线程析构
ThreadPool::~ThreadPool()
{}

//设置线程池工作模式
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

//设置task任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	if (checkRunningState())
		return;
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

	//cached模式需要根据任务数量和空闲线程的数量判断是否需要创建新的线程
	if (poolMode_ == PoolMode::MODE_CACHED
		&& taskSize_ > idleThreadSize_
		&& curThreadSize_ < threadSizeThreshHold_)
	{
		std::cout << "create new thread...." << std::endl;

		//创建新线程
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		//threads_.emplace_back(std::move(ptr));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));

		//启动线程
		threads_[threadId]->start();
		//修改线程个数相关变量
		curThreadSize_++;
		idleThreadSize_++;
	}

	//返回任务的Resu对象
	return Result(sp);
}

//开启线程
void ThreadPool::start(int initThreadSize)
{
	//设置线程池的运行状态
	isPoolRunning_ = true;

	//记录初始线程个数
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

	//创建线程对象
	for (int i = 0; i < initThreadSize_; i++)
	{
		//创建thread线程对象时，把线程函数给到thread对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		//threads_.emplace_back(std::move(ptr));

		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
	}
	//启动所有线程
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start(); //需要执行一个线程函数
		idleThreadSize_++;    //记录初始空闲线程数量
	}
}

//定义线程函数,消费任务
void ThreadPool::threadFunc(int threadid)
{
	auto lastTime = std::chrono::high_resolution_clock().now();

	for (;;)
	{
		std::shared_ptr<Task> task;
		{
			//先获取锁
			std::unique_lock<std::mutex>lock(taskQueMtx_);

			std::cout << "tid： " << std::this_thread::get_id()
				<<"尝试获取任务" << std::endl;

			//cached模式下，可能已经创建了很多线程，多余线程要回收
			//当前时间--上一次线程执行时间 > 60s
			if (poolMode_ == PoolMode::MODE_CACHED)
			{
				// 每一秒钟返回一次
				while (taskQue_.size() == 0)
				{
					//条件变量超时返回
					if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds> (now - lastTime);
						if (dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ > initThreadSize_)
						{
							//开始回收当前线程
							//记录线程数量相关变量修改
							//把线程从线程列表容器删除
							//threadid => thread对象=> 删除
							threads_.erase(threadid);
							curThreadSize_--;
							idleThreadSize_--;
							std::cout << "threadid:" << std::this_thread::get_id() << " exit！！" << std::endl;
							return;
						}
					
					}
				}
			}
			else
			{
				//等待notEmpty条件
				notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0; });
			}
			idleThreadSize_--;   

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
		idleThreadSize_++;
		//更新线程调度完执行的时间
		lastTime = std::chrono::high_resolution_clock().now();
	}

}

bool ThreadPool::checkRunningState() const
{

	return isPoolRunning_;
}

//------------线程方法实现------------

int Thread::generateId_ = 0;

//启动线程
void Thread::start()
{
	//创建一个线程来执行线程函数
	std::thread t(func_, threadId_);   //C++11,来说，线程对象t，和线程函数func_
	t.detach();    //设置分离线程   pthread_detach pthread_t 设置成分离线程
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
