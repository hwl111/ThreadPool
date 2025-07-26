#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<vector>
#include<queue>
#include<memory>
#include<atomic>
#include<mutex>
#include<condition_variable>
#include<functional>

// ����������
// �û������Զ�����������,��Task�̳У���дrun������ʵ�������Զ��崦��
class Task
{
public:
	virtual void run() = 0;
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
	using ThreadFunc = std::function<void()>;

	//�����߳�
	void start();

	Thread(ThreadFunc func);
	~Thread();


private:
	ThreadFunc func_;

};



//�̳߳���
class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();

	//�����̳߳ع���ģʽ
	void setMode(PoolMode mode);

	//����task�������������ֵ
	void setTaskQueMaxThreshHold(int threshhold);

	//���̳߳��ύ����
	void subnitTask(std::shared_ptr<Task> sp);

	//�����߳�
	void start(int initThreadSize = 4);

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool) = delete;

private:
	//�����̺߳���
	void threadFunc();

private:
	std::vector<Thread*> threads_;   //�߳��б�
	size_t initThreadSize_;          //��ʼ���߳�����

	std::queue<std::shared_ptr<Task>>taskQue_;  //������� 
	std::atomic_int taskSize_;                  //��������
	int taskQueMaxThreshHold_;                  //�����������������ֵ

	std::mutex taskQueMtx_;                     //��֤������а�ȫ
	std::condition_variable notFull_;   //������в���
	std::condition_variable notEmpty_;  //������в���

	PoolMode poolMode_;                 //��ǰ�̳߳صĹ���ģʽ
};

#endif
