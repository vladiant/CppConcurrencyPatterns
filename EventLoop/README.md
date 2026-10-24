# Event Loop

## Intent

An event loop serializes work onto a dedicated thread so that shared state is mutated from one place, in order, and without requiring every caller to coordinate manually.

The pattern is often used when a system has multiple producers of work but only one thread that should service those tasks.

## When to use

Use an event loop when:

- You want a single thread to own and process state transitions.
- Background tasks need to be queued and executed in FIFO order.
- GUI, UI, or I/O-driven code benefits from a single dispatcher thread.
- You want to avoid exposing locks directly to many callers.

This repository includes two practical examples:

- [Basic](./Basic/main.cpp) — a simple queue-based loop with enqueue, enqueueSync, and enqueueAsync APIs.
- [EventHandlers](./EventHandlers/event_loop.hpp) — the same loop pattern extended for handler-based event dispatch.

## Trade-offs

### Advantages

- Serializes access to shared state.
- Makes ordering explicit and predictable.
- Simplifies reasoning about cross-thread interactions.
- Works well for UI, networking, and callback-driven systems.

### Disadvantages

- The loop thread can become a bottleneck if work is CPU-heavy.
- Long-running tasks block the queue and delay later work.
- The pattern adds another layer of indirection and queueing latency.
- Debugging can be harder because control flow is implicit and event-driven.

## Minimal example

```cpp
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

class EventLoop {
public:
    void post(std::function<void()> fn) {
        {
            std::lock_guard<std::mutex> lock(m_);
            q_.push(std::move(fn));
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

## Benchmark guidance

Event loops are most effective when the work is naturally event-driven and relatively short-lived. They generally perform well for:

- many small tasks
- ordered execution
- decoupled producers and consumers

They are less attractive for:

- CPU-intensive work on the event thread
- large blocking operations in the loop
- workloads where the queue becomes the dominant latency source

A typical benchmark should compare:

- pure mutex + worker thread design
- event loop with queued callbacks
- active object / dedicated worker patterns

For quick validation, measure throughput and latency under the same task size, queue depth, and contention pattern. The event loop wins when ordering and determinism matter more than raw parallelism.

## References

- [Idiomatic Event Loop in C++](https://habr.com/en/post/665730/)
- [EventLoop examples in this repository](./Basic/main.cpp)
