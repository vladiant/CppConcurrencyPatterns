# Active Object

* Concurrent Architecture Pattern

* The active object pattern separates the method execution
from the method invocation.
  * Each object owns its own thread.
  * Each method invocation is stored in an activation list.
  * A scheduler triggers the method execution.

## Sequence Diagram

```
     ┌──────┐          ┌─────┐          ┌──────┐          ┌───────────────┐          ┌─────────┐          ┌───────┐          ┌───────┐
     │Client│          │Proxy│          │Future│          │Activation List│          │Scheduler│          │Request│          │Servant│
     └──┬───┘          └──┬──┘          └──┬───┘          └───────┬───────┘          └────┬────┘          └───┬───┘          └───┬───┘
       ┌┴┐   method()    ┌┴┐               │                      │                       │                   │                  │    
       │ │ ────────────> │ │               │                      │                       │                   │                  │    
       │ │               │ │               │                      │                       │                   │                  │    
       │ │               │ │   create()   ┌┴┐                     │                       │                   │                  │    
       │ │               │ │ ────────────>│ │                     │                       │                   │                  │    
       │ │               │ │              └┬┘                     │                       │                   │                  │    
       │ │               │ │     future    │                      │                       │                   │                  │    
       │ │               │ │ <─ ─ ─ ─ ─ ─ ─                       │                       │                   │                  │    
       │ │               │ │               │                      │                       │                   │                  │    
       │ │               │ │               │                 create(future)               │                  ┌┴┐                 │    
       │ │               │ │ ───────────────────────────────────────────────────────────────────────────────>│ │                 │    
       │ │               │ │               │                      │                       │                  └┬┘                 │    
       │ │               │ │               │                     request                  │                   │                  │    
       │ │               │ │ <─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ │                  │    
       │ │               │ │               │                      │                       │                   │                  │    
       │ │               │ │          enqueue(request)           ┌┴┐                      │                   │                  │    
       │ │               │ │ ───────────────────────────────────>│ │                      │                   │                  │    
       │ │               │ │               │                     └┬┘                      │                   │                  │    
       │ │               │ │               │                      │                      ┌┴┐                  │                  │    
       │ │               │ │ <─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ │                      │ │                  │                  │    
       │ │               │ │               │                      │                      │ │                  │                  │    
       │ │               │ │               │                     ┌┴┐      dequeue()      │ │                  │                  │    
       │ │               │ │               │                     │ │ <───────────────────│ │                  │                  │    
       │ │               │ │               │                     └┬┘                     │ │                  │                  │    
       │ │               │ │               │                      │      request         │ │                  │                  │    
       │ │               │ │               │                      │ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─> │ │                  │                  │    
       │ │               │ │               │                      │                      │ │                  │                  │    
       │ │               │ │               │                      │                      │ │     execute     ┌┴┐                 │    
       │ │               │ │               │                      │                      │ │ ───────────────>│ │                 │    
       │ │               │ │               │                      │                      │ │                 │ │                 │    
       │ │               │ │               │                      │                      │ │                 │ │    method()    ┌┴┐   
       │ │               │ │               │                      │                      │ │                 │ │ ──────────────>│ │   
       │ │               │ │               │                      │                      │ │                 │ │                └┬┘   
       │ │               │ │               │                      │                      │ │                 │ │     result      │    
       │ │               │ │               │                      │                      │ │                 │ │ <─ ─ ─ ─ ─ ─ ─ ─│    
       │ │               │ │               │                      │                      │ │                 │ │                 │    
       │ │               │ │              ┌┴┐                     │    setResult()       │ │                 │ │                 │    
       │ │               │ │              │ │ <───────────────────────────────────────────────────────────── │ │                 │    
       │ │               │ │              └┬┘                     │                      │ │                 │ │                 │    
       │ │               │ │               │                      │                      │ │                 │ │                 │    
       │ │               │ │               │  ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ >│ │                 │    
       │ │               │ │               │                      │                      └┬┘                 └┬┘                 │    
       │ │               │ │               │                      │                       │                   │                  │    
       │ │               │ │               │                      │                       │<─ ─ ─ ─ ─ ─ ─ ─ ─ │                  │    
       └┬┘               └┬┘               │                      │                       │                   │                  │    
        │                 │                │                      │                       │                   │                  │    
        │ <─ ─ ─ ─ ─ ─ ─ ─│                │                      │                       │                   │                  │    
        │                 │                │                      │                       │                   │                  │    
        │           getResult()           ┌┴┐                     │                       │                   │                  │    
        │ ───────────────────────────────>│ │                     │                       │                   │                  │    
        │                 │               └┬┘                     │                       │                   │                  │    
        │              result              │                      │                       │                   │                  │    
        │ <─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─                      │                       │                   │                  │    
     ┌──┴───┐          ┌──┴──┐          ┌──┴───┐          ┌───────┴───────┐          ┌────┴────┐          ┌───┴───┐          ┌───┴───┐
     │Client│          │Proxy│          │Future│          │Activation List│          │Scheduler│          │Request│          │Servant│
     └──────┘          └─────┘          └──────┘          └───────────────┘          └─────────┘          └───────┘          └───────┘
```

## Components

### Proxy
* Proxy for the member functions on the active object
* Triggers the construction of a request object which goes to the activation list and returns a future.
* It runs in the client thread.

### Method Request
* Includes all context information to be executed later.

### Activation List
* Has the pending requests objects.
* Decouples the client from the Active Object thread.

### Scheduler
* Runs in the thread of the Active Object.
* Decides with request from the Activation List is executes.

### Servant
* Implements the member functions of the active objects.
* Supports the interface of the Proxy.

### Future
* Is created by the Proxy.
* Is only necessary if the request objects returns a result.
* The client uses the future to get the result of the request object.

## Dynamic Behavior

### Request construction and scheduling
* The client invokes the method on the proxy.
* The proxy creates a request and passes it to the scheduler.
* The scheduler enqueues the request on the activation list and returns a future to the client if the request returns something.

### Member function execution
* The scheduler determines which request becomes runnable.
* It removes the request from the activation list and dispatches it to the servant.

### Completion
* Stores eventually the result of the request object in the future.
* Destructs the request object and the future when the client has the result.

## Advantages
* Only the access to the activation list has to be synchronized
* Clear separation of client and server
* Improved throughput due to the asynchronous execution
* The scheduler can use various execution policies.

## Disadvantages:
* If the member function execution is too fine-grained, the indirection may cause significant overhead.
*  The asynchronous member function execution and the various execution strategies make the system quite difficult to debug.