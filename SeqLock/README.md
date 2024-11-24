# Seqlock
* A special locking mechanism for supporting fast writes of shared variables between two threads. 
* It is used to avoid the problem of writer starvation.
* Used for data that is rarely written to.
* Reader never blocks and is willing to retry if that information changes.

## Implementation
* Writer increments the sequence number, both after acquiring the lock and before releasing the lock. 
* Readers read the sequence number before and after reading the shared data.
* If the sequence number is odd on either occasion, a writer had taken the lock while the data was being read and it may have changed.
* If the sequence numbers are different, a writer has changed the data while it was being read.
n either case readers simply retry until they read the same even sequence number before and after.

## Considerations
* Seqlocks are more efficient than traditional read-write locks for the situation where there are many readers and few writers.
* If there is too much write activity or the reader is too slow, they might livelock and the readers may starve.
* Does not work for data that contains pointers, because any writer could invalidate a pointer that a reader has already followed.
* Impossible to step through it with a debugger!
* The sanitizers and [helgrind](https://valgrind.org/docs/manual/hg-manual.html) tool will always suspect a data race due to missing lock!

## Fuzz test arbitrary times
| fuzz test Release build        | execution time |
| ------------------------------ | -------------- |
| test_seqlock_mutex_emulator    |  96.07 sec     |
| test_seqlock_emulator          | 114.24 sec     |
| test_seqlock_posix_emulator    | 116.49 sec     |
| test_seqlock_naive             |   2.45 sec     |
| test_seqlock_spinlock          |   0.64 sec     |
| test_seqlock_rigtorp           |   1.16 sec     |

## Perf data
* Command
```
perf stat -e instructions,cpu-cycles,L1-dcache-loads,L1-dcache-load-misses,mem_inst_retired.lock_loads,duration_time
```

| Example Release build     | instructions | cpu-cycles | L1-dcache-loads | L1-dcache-load-misses | mem_inst_retired.lock_loads | duration_time |
| ------------------------- | ------------ | ---------- | --------------- | --------------------- | --------------------------- | ------------- |
| seqlock_mutex_emulator    | 4152610      | 4832960    | 1057729         | 53110                 | 11503                       | 1818663 ns    |
| seqlock_emulator          | 4281531      | 4751058    | 1090099         | 54095                 | 13829                       | 3070747 ns    |
| seqlock_posix_emulator    | 4176506      | 4689458    | 1064256         | 53317                 | 11170                       | 2992380 ns    |
| seqlock_naive             | 4222576      | 4598630    | 1092859         | 53092                 |  9377                       | 2904287 ns    |
| seqlock_spinlock          | 4543389      | 4671161    | 1212866         | 53894                 |  9323                       | 2933602 ns    |
| seqlock_rigtorp           | 4394077      | 4769689    | 1147175         | 53246                 |  9732                       | 2724176 ns    |

## References
* [Wikipedia Seqlock](https://en.wikipedia.org/wiki/Seqlock)
* [Sequence counters and sequential locks](https://docs.kernel.org/locking/seqlock.html)
* [std::shared_mutex](https://en.cppreference.com/w/cpp/thread/shared_mutex)
* [std::shared_lock](https://en.cppreference.com/w/cpp/thread/shared_lock/lock)
* [Synchronized Output Streams with C++20](https://www.linkedin.com/pulse/synchronized-outputstreams-c20-rainer-grimm)
* [StackOverflow: how to implement a seqlock lock using c++11 atomic library](https://stackoverflow.com/questions/20342691/how-to-implement-a-seqlock-lock-using-c11-atomic-library)
* <https://github.com/rigtorp/Seqlock/>
* [std::hardware_destructive_interference_size](https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size)
