#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<vector>
#include<queue>
#include<memory>
#include<atomic>
#include<mutex>
#include<condition_variable>
#include<functional>
#include<unordered_map>
#include<thread>
//Task类型的前置声明
class Task;

//Any类型:表示可以接收任意数据的类型
class Any
{
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator = (const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	//这个构造函数让Any类型接收任意的数据
	template<typename T>
	Any(T data) :base_(std::make_unique<Derive<T>>(data))
	{}

	//这个方法能把Any对象存储的data数据提取出来
	template<typename T>
	T cast_()
	{
		//从base_找到他所指向的派生对象，提取data成员变量
		//基类指针---》派生指针 RTTI
		Derive<T> *pd = dynamic_cast<Derive<T>*>(base_.get());
		if (pd == nullptr)
		{
			throw "type is unmatch!!!";
		}
		return pd->data_;
	}

private:
	//基类类型
	class Base
	{
	public:
		virtual ~Base() = default;
	};

	//派生类类型
	template<typename T>
	class Derive :public Base
	{
	public:
		Derive(T data):data_(data)
		{}

		T data_;  //保存任意的其他类型
	};
private:
	//定义一个基类指针
	std::unique_ptr<Base> base_;
};

//实现一个信号量类
class Semaphore
{
public:
	Semaphore(int limit=0)
		:resLimit_(limit)
		,isExit_(false)
	{}
	~Semaphore()
	{
		isExit_ = true;
	}

	//获取一个信号量资源
	void wait()
	{
		if (isExit_)
		{
			return;   //防止在linux下阻塞
		}
		std::unique_lock<std::mutex>lock(mtx_);
		//等待信号量资源,没有资源会阻塞当前进程
		cond_.wait(lock, [&]()->bool {return resLimit_ > 0; });
		resLimit_--;
	}

	//增加一个信号量资源
	void post()
	{
		if (isExit_)
		{
			return;   //防止在linux下阻塞
		}
		std::unique_lock<std::mutex>lock(mtx_);
		resLimit_++;
		//linux下condition_variable的析构函数什么都没做
		//导致这里状态失效,无故阻塞
		cond_.notify_all();
	}

private:
	std::atomic_bool isExit_;
	int resLimit_;
	std::mutex mtx_;
	std::condition_variable cond_;
};


//实现接收提交线程池的task任务执行完后的返回值Result
class Result
{
public:
	Result(Result&&) = default;
	Result& operator=(Result&&) = default;
	Result(const Result&) = delete;
	Result& operator=(const Result&) = delete;

	Result(std::shared_ptr<Task>task, bool isValid = true);
	~Result() = default;

	//setVal方法,获取任务执行完的返回值
	void setVal(Any any);

	//get方法，用户调用这个方法获取task的返回值
	Any get();

private:
	Any any_;   //存储任务的返回值
	Semaphore sem_;    //线程通信信号量
	std::shared_ptr<Task>task_;   //指向对应获取任务的返回对象
	std::atomic_bool isValid_;    //返回是否有效


};


// 任务抽象基类
// 用户可以自定义任务类型,从Task继承，重写run方法，实现任务自定义处理
class Task
{
public:
	Task();
	~Task() = default;
	void exec();
	void setResult(Result* res);
	virtual Any run() = 0;
private:
	Result* result_;   // Result对象生命周期长于Task
};


// 线程池支持的模式
enum class PoolMode
{
	MODE_FIXED,   // 固定数量的线程
	MODE_CACHED,  // 线程数量可动态增长
};


//线程类型
class Thread
{
public:
	//线程函数对象类型
	using ThreadFunc = std::function<void(int)>;

	//启动线程
	void start();

	Thread(ThreadFunc func);
	~Thread();

	//获取线程ID
	int getId()const;

private:
	ThreadFunc func_;
	static int generateId_;

	int threadId_;   //保存线程ID

};

/*
exampl:
ThreadPool pool;
pool.stzrt(4);

class MyTask : public Task
{
	public:
		void run{// 编写代码}
};
pool.submitTask(std::make_shared<MyTask>());

*/

//线程池类
class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();

	//设置线程池工作模式
	void setMode(PoolMode mode);

	//设置线程池cached模式下线程阈值
	void setThreadSizeThreshHold(int threshhold);

	//设置task任务队列上限阈值
	void setTaskQueMaxThreshHold(int threshhold);

	//给线程池提交任务
	Result submitTask(std::shared_ptr<Task> sp);

	//开启线程
	void start(int initThreadSize = std::thread::hardware_concurrency());

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool) = delete;

private:
	//定义线程函数
	void threadFunc(int threadid);

	//检查Pool的运行状态
	bool checkRunningState() const;

private:
	//std::vector<std::unique_ptr<Thread>> threads_;   //线程列表
	std::unordered_map<int, std::unique_ptr<Thread>> threads_;    //线程列表
	
	size_t initThreadSize_;          //初始的线程数量

	std::atomic_int curThreadSize_;             //记录当前线程的总数量
	int threadSizeThreshHold_;                  //线程数量上限阈值
	//记录空闲线程数量
	std::atomic_int idleThreadSize_;

	std::queue<std::shared_ptr<Task>>taskQue_;  //任务队列 
	std::atomic_int taskSize_;                  //任务数量
	int taskQueMaxThreshHold_;                  //任务队列数量上限阈值

	std::mutex taskQueMtx_;                     //保证任务队列安全
	std::condition_variable notFull_;   //任务队列不满
	std::condition_variable notEmpty_;  //任务队列不空
	std::condition_variable exitCond_;  //等待线程资源全部回收

	PoolMode poolMode_;                 //当前线程池的工作模式

	//表示当前线程的启动状态
	std::atomic_bool isPoolRunning_;
};

#endif
