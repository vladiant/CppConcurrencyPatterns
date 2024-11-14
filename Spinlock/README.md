# Spinlock
* A lock that causes a thread trying to acquire it to simply wait in a loop ("spin") while repeatedly checking whether the lock is available.

## Considerations
* Efficient if threads are likely to be blocked for only short periods.
* Wasteful if held for longer durations
* Take into account the possibility of simultaneous access to the lock, which could cause race conditions.
* Implementation is possible with atomic test-and-set operations.
* Consider using exponential back-off to reduce load during wait.

## References
* [Wikipedia Spinlock](https://en.wikipedia.org/wiki/Spinlock)
* [OSDev Spinlock](https://wiki.osdev.org/Spinlock)
* [Using locks in real-time audio processing, safely](https://timur.audio/using-locks-in-real-time-audio-processing-safely)
* [Implementing a spinlock in c++](https://medium.com/@amishav/implementing-a-spinlock-in-c-8078ec584efc)
* [Correctly implementing a spinlock in C++](https://rigtorp.se/spinlock/)
* [Spinlocks - Part 7](https://coffeebeforearch.github.io/2020/11/07/spinlocks-7.html)
