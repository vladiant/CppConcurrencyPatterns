#include <condition_variable>
#include <functional>
#include <queue>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <concepts>

template <typename T>
class Monitor {
public:
    void lock() const {
        monitMutex.lock();
    }
    void unlock() const {
        monitMutex.unlock();
    }
    
    void notify_one() const noexcept {
        monitCond.notify_one();
    }

    template <std::predicate Predicate>
    void wait(Predicate pred) const {
        std::unique_lock<std::mutex> monitLock(monitMutex);
        monitCond.wait(monitLock, pred);
    }
    
private:
    mutable std::mutex monitMutex;
    mutable std::condition_variable monitCond;
};

template <typename T>
class ThreadSafeQueue: public Monitor<ThreadSafeQueue<T>>{
public:
    void add(T val){ 
        monitorForQueue.lock();
        myQueue.push(val);
        monitorForQueue.unlock();
        monitorForQueue.notify_one();
    }
    
    T get(){ 
        monitorForQueue.wait( [this] { return ! myQueue.empty(); } );
        monitorForQueue.lock();
        auto val = myQueue.front();
        myQueue.pop();
        monitorForQueue.unlock();
        return val;
    }

private:
    std::queue<T> myQueue;
    ThreadSafeQueue<T>& monitorForQueue = static_cast<ThreadSafeQueue<T>&>(*this);
};


class Dice {
public:
    int operator()(){ return rand(); }
private:
    std::function<int()> rand = std::bind(std::uniform_int_distribution<>(1, 6), 
                                          std::default_random_engine());
};


int main(){
    
    std::cout << '\n';
    
    constexpr auto NumberThreads = 10;
    
    ThreadSafeQueue<int> safeQueue;

    auto addLambda = [&safeQueue](int val){ safeQueue.add(val); 
                                            std::cout << val << " "
                                            << std::this_thread::get_id() << "; ";
                                          };
    auto getLambda = [&safeQueue]{ safeQueue.get(); };

    std::vector<std::jthread> getThreads(NumberThreads);
    for (auto& thr: getThreads) thr = std::jthread(getLambda);

    std::vector<std::jthread> addThreads(NumberThreads);
    Dice dice;
    for (auto& thr: addThreads) thr = std::jthread(addLambda, dice() );
    
    std::cout << "\n\n";
     
}
