# Spinlock
* A lock that causes a thread trying to acquire it to simply wait in a loop ("spin") while repeatedly checking whether the lock is available.

## Considerations
* Efficient if threads are likely to be blocked for only short periods.
* Wasteful if held for longer durations
* Take into account the possibility of simultaneous access to the lock, which could cause race conditions.
* Implementation is possible with atomic test-and-set operations.
* Consider using exponential back-off to reduce load during wait.
* Measure!
* Don’t `std::thread::yield()` in a loop.
* Don’t sleep

## Thread problems check
* GCC sanitizer
```
   g++ main.cpp -fsanitize=thread -std=c++20 -o main.x
```

* Clang sanitizer (recommended)
```
   clang++ main.cpp -fsanitize=thread -std=c++20 -o main.x
```

```
   cmake .. -DUSE_TSAN=ON -DCMAKE_C_COMPILER=/usr/bin/clang
```

```
   cmake .. -DUSE_TSAN=ON  -DCMAKE_CXX_COMPILER=/usr/bin/clang++
```


* Valgrind [Helgrind](https://valgrind.org/docs/manual/hg-manual.html)

   Produces false positives except for `pthread_spinlock_t`


## Benchmark arbitrary times
| Release build          | 1 core  | 2 cores | 3 cores   | 4 cores  |
| ---------------------- | --------| ------- | --------  |  ------- |
| audio_spin_mutex       | 3.90 ms | 1.86 ms | 13.7  ms  |  92.1 ms |
| spinlock_atomic_bool   | 1.22 ms | 3.16 ms | 21.2  ms  |  52.2 ms | 
| spinlock_atomic_flag   | 1.89 ms | 3.09 ms |  9.86 ms  |  39.0 ms |
| spinlock_atomic_flag_c | 2.53 ms | 7.86 ms | 36.8  ms  | 106.0 ms |
| spinlock_posix_c       | 1.47 ms | 2.96 ms | 18.4  ms  |  60.1 ms |

* <https://quick-bench.com/q/vJ2O7CXzMF6E_npt468jCEFTuiU>
* <https://godbolt.org/z/h9dxqaTsK>

## References
* [Wikipedia Spinlock](https://en.wikipedia.org/wiki/Spinlock)
* [OSDev Spinlock](https://wiki.osdev.org/Spinlock)
* [Using locks in real-time audio processing, safely](https://timur.audio/using-locks-in-real-time-audio-processing-safely)
* [Implementing a spinlock in c++](https://medium.com/@amishav/implementing-a-spinlock-in-c-8078ec584efc)
* [Correctly implementing a spinlock in C++](https://rigtorp.se/spinlock/)
* [Spinlocks - Part 7](https://coffeebeforearch.github.io/2020/11/07/spinlocks-7.html)
* <https://github.com/CoffeeBeforeArch/spinlocks/>

