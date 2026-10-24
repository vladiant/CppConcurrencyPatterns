# C++ Concurrency Patterns

This repository collects practical concurrency patterns for C++ and frames each one around the same core questions:

- What problem is the pattern trying to solve?
- When should it be chosen?
- What are the trade-offs and failure modes?
- What does the minimal structure look like?
- What does a benchmark tell us?

The patterns below are grouped by the kind of problem they usually solve: sharing state, mutating shared state, or structuring concurrent systems.

## Synchronization patterns

### Dealing with sharing

#### Copied Value
- Intent: Make ownership and lifetime explicit by copying data to the thread that needs it instead of sharing mutable memory.
- When to use: The data is small, immutable after creation, or easy to serialize between workers; you want to avoid races without complex locking.
- Trade-offs: Fast and simple, but copies can dominate runtime cost for large objects, and you lose shared-state semantics.
- Minimal example:
```cpp
#include <iostream>
#include <thread>
#include <string>

int main() {
    std::string payload = "hello";

    std::thread t([payload]() mutable {
        payload += " world";
        std::cout << payload << '\n';
    });

    t.join();
}
```
- Benchmark: Best when the object is cheap to copy; avoid for large structs or deep graphs. Benchmark the copy cost against a shared-mutable design before choosing it.

#### Thread-Specific Storage
- Intent: Give each thread its own private copy of state while preserving a global-looking API.
- When to use: Thread-local caches, logging, per-thread RNG state, or adapting a single-threaded design into a multi-threaded one.
- Trade-offs: Removes contention on the state itself, but can increase memory use and complicate debugging when thread-local state is hidden behind globals.
- Minimal example:
```cpp
#include <iostream>
#include <thread>

thread_local int counter = 0;

void worker() {
    ++counter;
    std::cout << "thread-local counter: " << counter << '\n';
}

int main() {
    std::thread t1(worker);
    std::thread t2(worker);
    t1.join();
    t2.join();
}
```
- Benchmark: Very low contention and usually excellent cache locality; cost is dominated by TLS setup and any per-thread initialization.

#### Future
- Intent: Represent a result that will be produced asynchronously and allow waiting only when needed.
- When to use: Background tasks, asynchronous computations, producer/consumer hand-offs, or work that is naturally split into a task and a result.
- Trade-offs: Clear ownership of results, but waiting on `get()` can block; scheduling overhead is non-trivial for tiny tasks.
- Minimal example:
```cpp
#include <future>
#include <iostream>
#include <thread>

int compute() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return 42;
}

int main() {
    std::future<int> f = std::async(std::launch::async, compute);
    std::cout << "result: " << f.get() << '\n';
}
```
- Benchmark: Best for coarse-grained tasks where the work is much larger than the scheduling cost; avoid for very small operations.

### Dealing with mutation

#### Scoped Locking
- Intent: Tie lock acquisition and release to object lifetime using RAII.
- When to use: Any critical section that must be protected by a mutex or lock guard and should never be left accidentally unlocked.
- Trade-offs: Very safe and easy to reason about, but the lock lifetime must match the critical section precisely; lock ordering still matters for deadlock avoidance.
- Minimal example:
```cpp
#include <mutex>
#include <iostream>

std::mutex m;
int value = 0;

void increment() {
    std::lock_guard<std::mutex> lock(m);
    ++value;
    std::cout << value << '\n';
}
```
- Benchmark: Adds minimal overhead beyond the actual lock acquisition; the real cost is contention under high parallelism.

#### Strategized Locking
- Intent: Select the locking strategy as a replaceable component, depending on the access pattern or workload.
- When to use: Libraries that might need exclusive locking, shared locking, or lock-free access under different deployment constraints.
- Trade-offs: Excellent flexibility, but the locking abstraction can become more complex and harder to maintain if the strategy choices grow too broad.
- Minimal example:
```cpp
#include <shared_mutex>
#include <mutex>

struct ReadWritePolicy {
    using mutex_type = std::shared_mutex;
};

template <typename Policy>
class Widget {
    typename Policy::mutex_type m;
};
```
- Benchmark: Compile-time strategies are typically the cheapest; runtime-selected strategies add a small indirection cost but improve flexibility.

#### Thread-Safe Interface
- Intent: Ensure that all public operations on an object obey a single synchronization discipline.
- When to use: Shared mutable objects with multiple methods that must preserve invariants and uphold consistent state transitions.
- Trade-offs: Strong correctness, but serialization can become a bottleneck; method call composition must avoid deadlocks or recursive locking.
- Minimal example:
```cpp
#include <mutex>

class BankAccount {
public:
    void deposit(int amount) {
        std::lock_guard<std::mutex> lock(m_);
        balance_ += amount;
    }

    int balance() const {
        std::lock_guard<std::mutex> lock(m_);
        return balance_;
    }

private:
    mutable std::mutex m_;
    int balance_ = 0;
};
```
- Benchmark: Serializes access to the object and is usually slower than lock-free single-variable updates under heavy contention.

#### Guarded Suspension
- Intent: Make a thread wait until a condition is satisfied, then resume under protection of a lock.
- When to use: Producer/consumer queues, work-stealing, or situations where a thread must block until a state transition occurs.
- Trade-offs: Correct and expressive, but adds scheduler and wakeup latency; incorrect condition checking leads to missed wakeups or spurious behavior.
- Minimal example:
```cpp
#include <condition_variable>
#include <mutex>
#include <queue>

std::mutex m;
std::condition_variable cv;
std::queue<int> q;

void producer() {
    std::lock_guard<std::mutex> lock(m);
    q.push(42);
    cv.notify_one();
}

void consumer() {
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [] { return !q.empty(); });
    int value = q.front();
    q.pop();
}
```
- Benchmark: Best when work arrives in bursts and waiting is expected; adds wakeup overhead compared with spin-based designs.

## Concurrent architecture patterns

#### Active Object
- Intent: Decouple method invocation from method execution by sending work to a dedicated worker thread.
- When to use: Long-running object operations, command queues, or APIs that should serialize access behind a single worker.
- Trade-offs: Strong isolation and easier reasoning, but adds queueing latency and can become a throughput bottleneck if the worker is overloaded.
- Minimal example:
```cpp
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

class ActiveObject {
public:
    template <typename F>
    auto enqueue(F&& f) -> std::future<decltype(f())> {
        std::packaged_task<decltype(f())()> task(std::forward<F>(f));
        auto future = task.get_future();
        { std::lock_guard<std::mutex> lock(m_); q_.push(std::move(task)); }
        cv_.notify_one();
        return future;
    }

    ActiveObject() : worker_([this] { loop(); }) {}
    ~ActiveObject() { { std::lock_guard<std::mutex> lock(m_); stop_ = true; } cv_.notify_one(); worker_.join(); }

private:
    void loop() {
        for (;;) {
            std::packaged_task<int()> task;
            {
                std::unique_lock<std::mutex> lock(m_);
                cv_.wait(lock, [this] { return stop_ || !q_.empty(); });
                if (stop_ && q_.empty()) return;
                task = std::move(q_.front());
                q_.pop();
            }
            task();
        }
    }

    std::mutex m_;
    std::condition_variable cv_;
    std::queue<std::packaged_task<int()>> q_;
    std::thread worker_;
    bool stop_ = false;
};
```
- Benchmark: Good throughput when tasks are independent and queueing is acceptable; main cost is serialization and worker latency.

#### Monitor Object
- Intent: Encapsulate state and synchronization behind a single lock/condition pair.
- When to use: Shared objects whose methods must run one at a time, while preserving a clean API boundary.
- Trade-offs: Simple design and predictable locking, but the monitor becomes the concurrency bottleneck and can interact poorly with nested calls.
- Minimal example:
```cpp
#include <mutex>
#include <condition_variable>

class Monitor {
public:
    void push(int x) {
        std::lock_guard<std::mutex> lock(m_);
        queue_.push_back(x);
        cv_.notify_one();
    }

    int pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [this] { return !queue_.empty(); });
        int v = queue_.front();
        queue_.pop_front();
        return v;
    }

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::vector<int> queue_;
};
```
- Benchmark: Excellent for correctness and simplicity; throughput is lower than lock-free structures when contention is high.

#### Reactor
- Intent: Demultiplex many event sources into a single event loop that dispatches work to handlers.
- When to use: Network servers, I/O-driven systems, or any workload where many independent events must be scheduled efficiently.
- Trade-offs: Excellent scalability for event-heavy workloads, but a long-running handler can block the loop and invert control flow.
- Minimal example:
```cpp
#include <functional>
#include <iostream>
#include <map>
#include <vector>

struct Event { int fd; };

class Reactor {
public:
    using Handler = std::function<void(const Event&)>;

    void add(int fd, Handler h) { handlers_[fd] = std::move(h); }
    void dispatch(const Event& e) { handlers_[e.fd](e); }

private:
    std::map<int, Handler> handlers_;
};

int main() {
    Reactor r;
    r.add(3, [](const Event& e) { std::cout << "handle fd " << e.fd << '\n'; });
    r.dispatch({3});
}
```
- Benchmark: Very strong for many low-cost events on one thread; poor if a handler blocks for a long time or performs CPU-heavy work.

#### Event Loop
- Intent: Serialize access to shared state by moving work onto one thread, which services a queue of callbacks.
- When to use: GUI/event-driven code, worker-thread orchestration, or background tasks that should be handled in order.
- Trade-offs: Simple programming model and predictable ordering, but the loop thread can become a bottleneck, and heavy work must be offloaded to other threads.
- Minimal example:
```cpp
#include <future>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

class EventLoop {
public:
    void enqueue(std::function<void()> f) {
        {
            std::lock_guard<std::mutex> lock(m_);
            q_.push(std::move(f));
        }
        cv_.notify_one();
    }

    void run() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_);
                cv_.wait(lock, [this] { return !q_.empty(); });
                task = std::move(q_.front());
                q_.pop();
            }
            task();
        }
    }

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> q_;
};
```
- Benchmark: High throughput for ordered work and smooth scheduling; avoid putting long tasks directly in the loop.

## Data flow and coordination patterns

#### Producer/Consumer
- Intent: Decouple data generation from data consumption with a queue or channel.
- When to use: Pipelines, log processing, I/O buffers, or any scenario where producers and consumers run at different rates.
- Trade-offs: Efficient buffering and clear separation of responsibilities, but the queue can become a source of backpressure or memory growth if not bounded.
- Minimal example:
```cpp
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

std::mutex m;
std::condition_variable cv;
std::queue<int> q;

void producer() {
    for (int i = 0; i < 10; ++i) {
        std::lock_guard<std::mutex> lock(m);
        q.push(i);
        cv.notify_one();
    }
}

void consumer() {
    for (int i = 0; i < 10; ++i) {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [] { return !q.empty(); });
        int value = q.front();
        q.pop();
    }
}
```
- Benchmark: Bounded queues improve backpressure control; unbounded queues are simpler but can hide latency spikes or memory pressure. See the examples in [ProducerConsumer/](./ProducerConsumer).

#### Spinlock
- Intent: Busy-wait while polling a lock flag until ownership is available.
- When to use: Very short critical sections, usually in kernel or real-time code, where the lock hold time is known to be tiny and waiting should avoid context switches.
- Trade-offs: Can be extremely fast for short locks, but is wasteful if the critical section is long and can create excessive cache traffic or CPU burn.
- Minimal example:
```cpp
#include <atomic>

class SpinLock {
public:
    void lock() {
        while (flag_.test_and_set(std::memory_order_acquire)) {
        }
    }

    void unlock() { flag_.clear(std::memory_order_release); }

private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};
```
- Benchmark: Best for tiny critical sections and low contention; poor when lock hold time grows or when many threads are contending. See [Spinlock/benchmarks](./Spinlock/benchmarks/README.md) and [Spinlock/benchmarks/main.cpp](./Spinlock/benchmarks/main.cpp).

#### Seqlock
- Intent: Favor very fast read access by allowing readers to retry if a writer changes the data while they are reading.
- When to use: Read-mostly shared state such as configuration values or status counters where writers are rare and latency-sensitive reads matter.
- Trade-offs: Excellent read-side performance, but readers may retry and the pattern is not a general-purpose replacement for a mutex; it also has well-known caveats around pointer-based data and write-heavy workloads.
- Minimal example:
```cpp
#include <atomic>

struct SeqValue {
    std::atomic<unsigned> seq{0};
    int value{0};
};

void write(SeqValue& v, int new_value) {
    v.seq.fetch_add(1, std::memory_order_relaxed);
    v.value = new_value;
    v.seq.fetch_add(1, std::memory_order_release);
}

int read(SeqValue& v) {
    unsigned s1 = v.seq.load(std::memory_order_acquire);
    if (s1 & 1u) return 0;
    int x = v.value;
    unsigned s2 = v.seq.load(std::memory_order_acquire);
    return (s1 == s2 && !(s1 & 1u)) ? x : read(v);
}
```
- Benchmark: Excellent under read-heavy traffic; under heavy writes or slow readers, retries can dominate. See [SeqLock/README.md](./SeqLock/README.md).

#### LockFree
- Intent: Design data structures that avoid blocking locks and rely on atomics and memory ordering instead.
- When to use: High-throughput queues or stacks, SPSC communication, or workloads where the serial bottleneck of a mutex is unacceptable.
- Trade-offs: Potentially highest throughput, but also the highest complexity: memory ordering bugs, ABA concerns, and difficult debugging are common.
- Minimal example:
```cpp
#include <atomic>
#include <thread>

struct Node { int value; std::atomic<Node*> next; };

std::atomic<Node*> head{nullptr};

void push(Node* n) {
    n->next = head.load(std::memory_order_relaxed);
    while (!head.compare_exchange_weak(n->next, n,
        std::memory_order_release, std::memory_order_relaxed)) {}
}
```
- Benchmark: Often wins for minimal, single-producer/single-consumer or very low-contention cases; complexity and correctness risk increase sharply for multi-threaded general-purpose usage. The repo includes [LockFree/Benchmark/main.cpp](./LockFree/Benchmark/main.cpp) and [LockFree/](./LockFree).

#### OpenMP
- Intent: Add parallel loops, reductions, and task parallelism with compiler directives instead of manually managing threads.
- When to use: Numerical kernels, data-parallel loops, and workloads where the compiler and runtime can handle simple parallelism without custom thread code.
- Trade-offs: Very concise for loop-based work, but large parallel regions may require careful attention to data sharing, reduction variables, and platform-specific performance.
- Minimal example:
```cpp
#include <omp.h>
#include <vector>

int main() {
    std::vector<int> values(1024);

#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(values.size()); ++i) {
        values[i] = i * i;
    }
}
```
- Benchmark: Outstanding for data-parallel numerical code; parallel overhead becomes noticeable for small loops or irregular work.

## Quick decision guide

- Need a single invariant or small critical section? Use Scoped Locking or a Thread-Safe Interface.
- Need a thread-local cache or per-thread state? Use Thread-Specific Storage.
- Need a result after work finishes? Use Future.
- Need a clean worker queue? Use Active Object or Event Loop.
- Need a network or event-driven server? Use Reactor.
- Need the fastest possible queue for a throughput-sensitive hot path? Consider LockFree or SPSC queue designs.
- Need response under very short critical sections? Consider Spinlock carefully.
- Need read-mostly data with rare writes? Consider Seqlock.
- Need data-parallel numerical work? OpenMP is often the fastest route to a working parallel loop.

## References
- [Concurrency Patterns - Rainer Grimm - CppCon 2021](https://www.youtube.com/watch?v=A3DQxZCtKqo)
- [Pattern-Oriented Software Architecture](https://en.wikipedia.org/wiki/Pattern-Oriented_Software_Architecture)
- [Concurrency with Modern C++](https://leanpub.com/concurrencywithmodernc/c/RkLJ8CTGGIo2)
- [Idiomatic Event Loop in C++](https://habr.com/en/post/665730/)
- [OpenMP: Home](https://www.openmp.org/)
