#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

using namespace std;

//构建安全消息队列
template <typename MsgType>
class MsgQueue{
private:
    queue<MsgType> _queue;
    mutex _mutex; //用一个mutex来保证访问队列的线程安全
    condition_variable _enqCv; //入队条件变量，当队列满了，用来阻塞生产者线程
    condition_variable _deqCv; //出队条件变量，当队列空了，用来阻塞消费者线程
    int _limit;
public:
    MsgQueue(int limit=3):_limit(limit){}
    //将消息放入队列
    void Enqueue(MsgType& msg){
        unique_lock<mutex> lock(_mutex);
        if(_queue.size()>=_limit){
            cout << "queue is full, wait() ... " << endl;
            _enqCv.wait(lock);
        }
        _queue.push(msg);
        //唤醒消费者线程
        _deqCv.notify_one();
    }
    //将消息从队列中取出
    MsgType Dequeue(){
        unique_lock<mutex> lock(_mutex);
        if(_queue.empty()){
            cout << " queue is empty , wait() ... " << endl;
            //消费者线程被堵塞
            _deqCv.wait(lock);
        }
        MsgType  msg = _queue.front();
        _queue.pop();
        //唤醒正在因为队列已满被阻塞的生产者线程
        _enqCv.notify_one();
        return msg;
    }
    int Size(){
        unique_lock<mutex> lock(_mutex);
        return _queue.size();
    }
};

struct CustomerTask
{
    string task;
    float money;
    CustomerTask(){}
    CustomerTask(const CustomerTask& cp):task(cp.task),money(cp.money){}
    void ExecuteTask()
    {
        if(money>0)
            cout<<"Task "<<task<<" is executed at $"<<money<<endl;
        else
            cout<<"Bank closed because the price is $"<<money<<endl;
    }
};

typedef MsgQueue<CustomerTask> TaskQueueType;
//构建线程池
class ThreadPool
{
private:
    int _limit;  //线程池中线程数量的上限
    vector<thread*> _workerThreads; //存储所有线程的容器。
    TaskQueueType& _taskQueue; // 安全队列的引用，用于存储任务
    bool _threadPoolStop=false; //线程池停止flag
public:
    //定义执行任务的方法
    void ExecuteTask()
    {
        while(1)
        {
            //不断的从任务队列中取出任务执行
            CustomerTask task=_taskQueue.Dequeue();
            //执行任务
            task.ExecuteTask();
            if(task.money<0)
            {
                _threadPoolStop=true;
                //向任务队列重新加入该任务，告知其他线程停止
                _taskQueue.Enqueue(task);
            }
            if(_threadPoolStop)
            {
                cout<< "thread finshed!"<<endl;
                return;
            }
            //the sleep function simulates that the task takes a while to finish 
            std::this_thread::sleep_for(std::chrono::milliseconds(rand()%100));
        }
    }
    //构造函数
    ThreadPool(TaskQueueType& taskQueue,int limit=3):_limit(limit),_taskQueue(taskQueue){
        //根据初始化的线程数，将new thread 放入vector中
        for(int i=0;i<_limit;++i)
        {
            //每个线程执行 ExecuteTask 方法
            _workerThreads.push_back(new thread(&ThreadPool::ExecuteTask,this));
        }
    }
    //析构函数
    ~ThreadPool(){
        for(auto threadObj: _workerThreads)
        {
            if(threadObj->joinable())
                {
                    threadObj->join();
                    delete threadObj;
                }
        }
    }
};


void TestLeaderFollower()
{
    //创建一个安全队列，limit为5
    TaskQueueType taskQueue(5);
    //创建一个线程池，线程数量为3，并传入安全队列taskQueue
    ThreadPool pool(taskQueue,3);
    //the sleep function simulates the situation that all  
    //worker threads are waiting for the empty message queue at the beginning
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    for(int i=0;i<10;i++)
    {
        CustomerTask task;
        task.money= i+1;
        if(task.money >5)
            task.task = "deposit $";
        else
            task.task = "withdraw $";    
        taskQueue.Enqueue(task);
    }
    CustomerTask bankClosedTask;
    bankClosedTask.task="Bank Closed!";
    bankClosedTask.money=-1;
    taskQueue.Enqueue(bankClosedTask);
}
int main()
{
    TestLeaderFollower();
    return 0;
}



