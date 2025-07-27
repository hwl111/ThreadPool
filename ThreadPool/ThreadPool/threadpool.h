#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<vector>
#include<queue>
#include<memory>
#include<atomic>
#include<mutex>
#include<condition_variable>
#include<functional>

// 任务抽象基类
// 用户可以自定义任务类型,从Task继承，重写run方法，实现任务自定义处理
class Task
{
public:
	virtual void run() = 0;
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
	using ThreadFunc = std::function<void()>;

	//启动线程
	void start();

	Thread(ThreadFunc func);
	~Thread();


private:
	ThreadFunc func_;

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

	//设置task任务队列上限阈值
	void setTaskQueMaxThreshHold(int threshhold);

	//给线程池提交任务
	void submitTask(std::shared_ptr<Task> sp);

	//开启线程
	void start(int initThreadSize = 4);

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool) = delete;

private:
	//定义线程函数
	void threadFunc();

private:
	std::vector<std::unique_ptr<Thread>> threads_;   //线程列表
	size_t initThreadSize_;          //初始的线程数量

	std::queue<std::shared_ptr<Task>>taskQue_;  //任务队列 
	std::atomic_int taskSize_;                  //任务数量
	int taskQueMaxThreshHold_;                  //任务队列数量上限阈值

	std::mutex taskQueMtx_;                     //保证任务队列安全
	std::condition_variable notFull_;   //任务队列不满
	std::condition_variable notEmpty_;  //任务队列不空

	PoolMode poolMode_;                 //当前线程池的工作模式
};

#endif
