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

## References
* [Wikipedia Seqlock](https://en.wikipedia.org/wiki/Seqlock)
* [Sequence counters and sequential locks](https://docs.kernel.org/locking/seqlock.html)
