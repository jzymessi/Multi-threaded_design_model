#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

template <typename T>
class MutexSafe{

private:
    std::mutex _mutex;
    T* _resource;
    T* operator ->(){}
    T* operator &(){}
public:
    MutexSafe(T* resource):_resource(resource){}
    ~MutexSafe(){delete _resource;}

    void lock(){
        _mutex.lock();
    }
    void unlock(){
        _mutex.unlock();
    }
    bool try_lock(){
        return _mutex.try_lock();
    }
    mutex& Mutex(){
        return _mutex;
    }
    
    //接收了一个unique_lock<SafeT>参数，这就保证了程序员在使用的时候必须传入一个锁对象
    template <class SafeT>
    T& Acquire (unique_lock<SafeT>& lock)  
    {
        return *_resource;
    }


};

void DannyWrite(string& blackboard)
{
    blackboard+="My";
    this_thread::sleep_for(std::chrono::milliseconds(rand()%3));
    blackboard+= " name";
    this_thread::sleep_for(std::chrono::milliseconds(rand()%3));
    blackboard+=" is";
    this_thread::sleep_for(std::chrono::milliseconds(rand()%3));
    blackboard+= " Danny\n";
}
void PeterWrite(string& blackboard)
{
    blackboard+="My";
    this_thread::sleep_for(std::chrono::milliseconds(rand()%3));
    blackboard+= " name";
    this_thread::sleep_for(std::chrono::milliseconds(rand()%3));
    blackboard+=" is";
    this_thread::sleep_for(std::chrono::milliseconds(rand()%3));
    blackboard+= " Peter\n";
}

void SafeDannyWrite(MutexSafe<string>& safe)
{
    unique_lock<MutexSafe<string>> lock(safe);
    string& blackboard = safe.Acquire(lock);
    DannyWrite(blackboard);
}

void SafePeterWrite(MutexSafe<string>& safe)
{   
    //创建一个unique_lock,用来锁safe
    unique_lock<MutexSafe<string>> lock(safe);
    //调用Acquire申请的资源
    string& blackboard = safe.Acquire(lock);
    PeterWrite(blackboard);
}

void DemoResourceLock(){
    MutexSafe<string> safe(new string());
    thread DannyThread(SafeDannyWrite,ref(safe));
    thread PeterThread(SafePeterWrite, ref(safe));
    DannyThread.join();
    PeterThread.join();
    //由于申请资源的上锁的资源，所以在主线程中进行访问也需要先lock,再调用申请的资源
    unique_lock<MutexSafe<string>> lock(safe);
    string& blackboard = safe.Acquire(lock);
    cout<<blackboard<<endl;
}

int main(){
    DemoResourceLock();
    return 0;
}

