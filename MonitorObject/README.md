# Monitor Object

* Concurrent Architecture Pattern

* The monitor object synchronizes the access to an object so that at most one member function can run at any moment in time.

## Summary

* Each object has a monitor lock and a monitor condition.

* The monitor lock guarantees that only one client can execute a member function of the object.

* The monitor condition notifies the waiting clients.

## Components

### Monitor Object 

*  Support member functions, which can run in the thread of the client.

### Synchronized Methods

* Interface member functions of the monitor object.
* At most, one member function can run at any point in time
* The member functions should apply the thread-safe interface pattern.

### Monitor Lock

* Each monitor object has a monitor lock.

* Guarantees exclusive access to the member functions.

### Monitor Condition

* Allows various threads to store their member function invocation.

* When the current thread is done with its member function execution, the next thread is awoken.

## Advantages

* The synchronization is encapsulated in the implementation.

* The member function execution is automatically stored and performed.

* The monitor object is a simple scheduler.

## Disadvantages

* The synchronization mechanism and the functionality are strongly coupled and can, therefore, not easily be changed.

* When the synchronized member functions invoke an additional member function of the monitor object, a deadlock may happen.
