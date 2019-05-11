#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<queue>
#include<time.h>

//实现一个线程池
//线程池由一定数量的线程和一个任务队列组成

typedef bool (*task_call)(int data); //定义了一个函数指针task_call
bool deal_data(int data)             //数据处理方法
{
	srand(time(NULL));               //初始化计数器
	int n = rand()%6;                //生成随机数
	printf("thread:%p---deal data:%d---sleep:%d sec\n",pthread_self(),data,n); //输出内核态线程ID，数据，失眠时间
	sleep(n);
	return true;
}

class Task
{
public:
	Task(){};
	~Task(){};
	void SetTask(int data,task_call handle){   //任务处理设置函数，包括数据和处理方法
		_data = data;
		_handle = handle;
	} 
	bool run(){                             //运行处理函数
		return _handle(_data);
	}
private:
	int _data;
	task_call _handle;
};

#define MAX_THR 5
#define MAX_QUE 10
class PthreadPool
{
public:
	PthreadPool(){	
	}
	~PthreadPool(){
		pthread_mutex_destroy(&mutex);
		pthread_cond_destroy(&consumor);
		pthread_cond_destroy(&productor);
	}
	bool ThreadInit(int max_thr = MAX_THR,int max_que = MAX_QUE)
	{
		quit_flag = false;
		_max_thr = max_thr;
		_cur_thr = max_thr;
		capacity = max_que;
		pthread_mutex_init(&mutex,NULL);
		pthread_cond_init(&consumor,NULL);
		pthread_cond_init(&productor,NULL);
		int ret;
		pthread_t tid;
		for(int i=0;i<_max_thr;i++){
			pthread_create(&tid,NULL,pth_start,(void*)this);
			if(ret!=0){
				printf("pthread create error!\n");
				return false;
			}
			pthread_detach(tid);
		}
		return true;
	}
	bool PushTask(Task& task){
		if(quit_flag==true){
			return false;
		}
		QueueLock();
		while(QueueIsFull()){
			ProWait();
		}
		queue.push(task);
		ConWakeUp();
		QueueUnlock();
		return true;
	}
	void ThreadQuit(){
		if(quit_flag!=true){
			quit_flag = true;
		}
		while(_cur_thr>0){
			ConWakeUpAll();
			sleep(1);
		}
		return; 
	}
private:
	void QueueLock(){
		pthread_mutex_lock(&mutex);

	}
	void QueueUnlock(){
		pthread_mutex_unlock(&mutex);
	}
	void ConWait(){
		if(quit_flag==true){
			pthread_mutex_unlock(&mutex);
			printf("thread:%p exit\n", pthread_self());
			_cur_thr--;
			pthread_exit(NULL);
		}
		pthread_cond_wait(&consumor,&mutex);
	}
	void ConWakeUp(){
		pthread_cond_signal(&consumor);
	}
	void ConWakeUpAll(){
		printf("wake up all!\n");
		pthread_cond_broadcast(&consumor);
	}
	void ProWait(){
		pthread_cond_wait(&productor,&mutex);
	}
	void ProWakeUp(){
		pthread_cond_signal(&productor);
	}
	bool QueueIsFull(){
		return (queue.size() == capacity);
	}
	bool QueueIsEmpty(){
		return queue.empty();
	}
	void PopTask(Task* task){
		*task = queue.front();
		queue.pop();
	}
	static void* pth_start(void* arg){
		PthreadPool* p = (PthreadPool*) arg;
		while(1){
			p->QueueLock();
			while(p->QueueIsEmpty()){
				p->ConWait();
			}
			Task task;
			p->PopTask(&task);
			p->ProWakeUp();
			p->QueueUnlock();
			task.run();
		}
		return NULL;
	} 

private:
	int _max_thr;
	int _cur_thr;
	int capacity;
	bool quit_flag;
	std::queue<Task> queue;
	pthread_mutex_t mutex;
	pthread_cond_t productor;
	pthread_cond_t consumor;
};
int main()
{
	PthreadPool p;
	p.ThreadInit();
	Task task[10];
	for(int i=0;i<10;i++){
		task[i].SetTask(i,deal_data);
		p.PushTask(task[i]);
	}
	p.ThreadQuit();
	return 0;
}
