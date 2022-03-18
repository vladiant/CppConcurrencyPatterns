# Scoped Locking

* Dealing with Mutation Pattern

* Scoped Locking is RAII applied to locking

## Idea:

* Bind the acquiring (constructor) and the releasing (destructor) of
the resource to the lifetime of an object.
* The lifetime of the object is bound.
* The C++ run time is responsible for invoking the destructor and
releasing the resource.

## C++ Implementation
* [std::lock_guard](https://en.cppreference.com/w/cpp/thread/lock_guard) and [std::scoped_lock](https://en.cppreference.com/w/cpp/thread/scoped_lock)
* [std::unique_lock](https://en.cppreference.com/w/cpp/thread/unique_lock) and [std::shared_lock](https://en.cppreference.com/w/cpp/thread/shared_lock)

