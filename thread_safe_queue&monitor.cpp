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

//定义msg的数据格式
struct StockPrice
{
    float price;
    const string name;
    StockPrice(const string stockName,float stockPrice = 0):name(stockName),price(stockPrice){}
    StockPrice(const StockPrice& stock):name(stock.name),price(stock.price){}
};

typedef MsgQueue<StockPrice> StockMsgQType;

//该线程主要用来向队列中不停的写
void StockPriceProducer(StockMsgQType& msgQueue){
    for(int i=0;i<10;i++){
        StockPrice stock("APPL",abs(rand()%100));
        msgQueue.Enqueue(stock);
        cout << i << ": Stock price $ " << stock.price << "is added to queue" << endl;

        this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    StockPrice endStock("APPL",-1);
    msgQueue.Enqueue(endStock);
    cout << "Announce market closed" << endl;
}
//该线程主要用来从队列中取数据进行分析
void DannyReadStock_sell(StockMsgQType& msgQueue){
    while(1){
        StockPrice stock = msgQueue.Dequeue();
        cout << "Danny Read stock Price: $" << stock.price;
        if(stock.price>=90){
            cout << ", sell at $" << stock.price << endl;
        }
        else if(stock.price<=0)
        {
            cout << ", Marked closed!" << endl;
            break;
        }
        else if(stock.price>0 && stock.price<=10){
            cout << ", bug at $" << stock.price << endl;
        }
        else{
            cout << ", do nothing..." << endl;
        }
        
    }

}

void TestStockMsgUpdate()
{
    MsgQueue<StockPrice> msgQ;
    thread StockPriceProducerThread(StockPriceProducer,std::ref(msgQ));
    thread DannyThread(DannyReadStock_sell,std::ref(msgQ));
    StockPriceProducerThread.join();
    DannyThread.join();
    
}
int main()
{
    TestStockMsgUpdate();
    return 0;
}



