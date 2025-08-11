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
//Task���͵�ǰ������
class Task;

//Any����:��ʾ���Խ����������ݵ�����
class Any
{
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator = (const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	//������캯����Any���ͽ������������
	template<typename T>
	Any(T data) :base_(std::make_unique<Derive<T>>(data))
	{}

	//��������ܰ�Any����洢��data������ȡ����
	template<typename T>
	T cast_()
	{
		//��base_�ҵ�����ָ�������������ȡdata��Ա����
		//����ָ��---������ָ�� RTTI
		Derive<T> *pd = dynamic_cast<Derive<T>*>(base_.get());
		if (pd == nullptr)
		{
			throw "type is unmatch!!!";
		}
		return pd->data_;
	}

private:
	//��������
	class Base
	{
	public:
		virtual ~Base() = default;
	};

	//����������
	template<typename T>
	class Derive :public Base
	{
	public:
		Derive(T data):data_(data)
		{}

		T data_;  //�����������������
	};
private:
	//����һ������ָ��
	std::unique_ptr<Base> base_;
};

//ʵ��һ���ź�����
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

	//��ȡһ���ź�����Դ
	void wait()
	{
		if (isExit_)
		{
			return;   //��ֹ��linux������
		}
		std::unique_lock<std::mutex>lock(mtx_);
		//�ȴ��ź�����Դ,û����Դ��������ǰ����
		cond_.wait(lock, [&]()->bool {return resLimit_ > 0; });
		resLimit_--;
	}

	//����һ���ź�����Դ
	void post()
	{
		if (isExit_)
		{
			return;   //��ֹ��linux������
		}
		std::unique_lock<std::mutex>lock(mtx_);
		resLimit_++;
		//linux��condition_variable����������ʲô��û��
		//��������״̬ʧЧ,�޹�����
		cond_.notify_all();
	}

private:
	std::atomic_bool isExit_;
	int resLimit_;
	std::mutex mtx_;
	std::condition_variable cond_;
};


//ʵ�ֽ����ύ�̳߳ص�task����ִ�����ķ���ֵResult
class Result
{
public:
	Result(Result&&) = default;
	Result& operator=(Result&&) = default;
	Result(const Result&) = delete;
	Result& operator=(const Result&) = delete;

	Result(std::shared_ptr<Task>task, bool isValid = true);
	~Result() = default;

	//setVal����,��ȡ����ִ����ķ���ֵ
	void setVal(Any any);

	//get�������û��������������ȡtask�ķ���ֵ
	Any get();

private:
	Any any_;   //�洢����ķ���ֵ
	Semaphore sem_;    //�߳�ͨ���ź���
	std::shared_ptr<Task>task_;   //ָ���Ӧ��ȡ����ķ��ض���
	std::atomic_bool isValid_;    //�����Ƿ���Ч


};


// ����������
// �û������Զ�����������,��Task�̳У���дrun������ʵ�������Զ��崦��
class Task
{
public:
	Task();
	~Task() = default;
	void exec();
	void setResult(Result* res);
	virtual Any run() = 0;
private:
	Result* result_;   // Result�����������ڳ���Task
};


// �̳߳�֧�ֵ�ģʽ
enum class PoolMode
{
	MODE_FIXED,   // �̶��������߳�
	MODE_CACHED,  // �߳������ɶ�̬����
};


//�߳�����
class Thread
{
public:
	//�̺߳�����������
	using ThreadFunc = std::function<void(int)>;

	//�����߳�
	void start();

	Thread(ThreadFunc func);
	~Thread();

	//��ȡ�߳�ID
	int getId()const;

private:
	ThreadFunc func_;
	static int generateId_;

	int threadId_;   //�����߳�ID

};

/*
exampl:
ThreadPool pool;
pool.stzrt(4);

class MyTask : public Task
{
	public:
		void run{// ��д����}
};
pool.submitTask(std::make_shared<MyTask>());

*/

//�̳߳���
class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();

	//�����̳߳ع���ģʽ
	void setMode(PoolMode mode);

	//�����̳߳�cachedģʽ���߳���ֵ
	void setThreadSizeThreshHold(int threshhold);

	//����task�������������ֵ
	void setTaskQueMaxThreshHold(int threshhold);

	//���̳߳��ύ����
	Result submitTask(std::shared_ptr<Task> sp);

	//�����߳�
	void start(int initThreadSize = std::thread::hardware_concurrency());

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool) = delete;

private:
	//�����̺߳���
	void threadFunc(int threadid);

	//���Pool������״̬
	bool checkRunningState() const;

private:
	//std::vector<std::unique_ptr<Thread>> threads_;   //�߳��б�
	std::unordered_map<int, std::unique_ptr<Thread>> threads_;    //�߳��б�
	
	size_t initThreadSize_;          //��ʼ���߳�����

	std::atomic_int curThreadSize_;             //��¼��ǰ�̵߳�������
	int threadSizeThreshHold_;                  //�߳�����������ֵ
	//��¼�����߳�����
	std::atomic_int idleThreadSize_;

	std::queue<std::shared_ptr<Task>>taskQue_;  //������� 
	std::atomic_int taskSize_;                  //��������
	int taskQueMaxThreshHold_;                  //�����������������ֵ

	std::mutex taskQueMtx_;                     //��֤������а�ȫ
	std::condition_variable notFull_;   //������в���
	std::condition_variable notEmpty_;  //������в���
	std::condition_variable exitCond_;  //�ȴ��߳���Դȫ������

	PoolMode poolMode_;                 //��ǰ�̳߳صĹ���ģʽ

	//��ʾ��ǰ�̵߳�����״̬
	std::atomic_bool isPoolRunning_;
};

#endif
