# Thread-Specific Storage

* Dealing with Mutation Pattern

* A guarded suspension consists of a lock and a condition, which has to be fulfilled by the calling thread.

## Usage:

* The calling thread will put itself to sleep if the condition is not
meet.

* The calling thread uses a lock when it checks the condition.

* The lock protects the calling thread from a data race or deadlock.

## Variations:

### The waiting thread is notified about the state change or asks for the state change.

* Push principle: condition variables, future/promise pairs, atomics (C++20), and semaphores (C++20)

* Pull principle: not natively supported in C++

### The waiting thread waits with or without a time limit.

* Condition variables, and future/promise pairs

### The notification is sent to one or all waiting threads.

* Shared futures, condition variables, atomics (C++20), and semaphores (C++20)