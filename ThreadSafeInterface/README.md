# Thread-Safe Interface

* Dealing with Mutation Pattern

* The thread-safe interface extends the critical region to an
object.

## Antipattern: Each member function uses a lock internally

* The performance of the system goes down.
* Deadlocks appear when two member functions call each other.

## Solutions:

* All interface member functions (public) use a lock.

* All implementation member functions (protected and
private) must not use a lock.

* The interface member functions call only implementation member
functions.
