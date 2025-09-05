## Producer/Consumer pattern
The Producer/Consumer pattern is used to decouple processes that produce and consume data at different rates. The Producer/Consumer pattern's parallel loops are broken down into two categories; those that produce data, and those that consume the data produced.

### Dijkstra's bounded buffer
* The buffer can store N portions or elements.
* The "number of queueing portions" semaphore counts the filled locations in the buffer
* The "number of empty positions" semaphore counts the empty locations in the buffer
* The "buffer manipulation" semaphore works as mutex for the buffer put and get operations.

### Using monitors
Monitor is a shared variable and the set of meaningful operations on it. It is used to control the scheduling of resources among individual processes.

### Using channels
An alternative to explicit naming of source and destination would be to name a port through which communication is to take place.

### Lamport's without semaphores
Bounded buffer solution for one producer and one consumer.

## Reader/Writer pattern
The Reader-Writer pattern addresses scenarios where multiple threads read shared data, but only one thread can write at a time.

### First readers-writers problem
No reader shall be kept waiting if the share is currently opened for reading (readers-preference).

### Second readers-writers problem
No writer, once added to the queue, shall be kept waiting longer than absolutely necessary (writers-preference). 

### Third readers-writers problem
No thread shall be allowed to starve

### Simplest reader writer problem
Uses only two semaphores and doesn't need an array of readers to read the data in buffer.

## Single Producer Single Consumer
* [Building Robust Inter-Process Queues in C++ - Jody Hagins - C++ on Sea 2025](https://www.youtube.com/watch?v=nX5CXx1gdEg)
* [Writing a Fast and Versatile SPSC Ring Buffer](https://daugaard.org/blog/writing-a-fast-and-versatile-spsc-ring-buffer/)
* <https://github.com/jodyhagins/DaugaardRingBuffer>
* <https://github.com/jodyhagins/RigtorpSPSCQueue>
* <https://github.com/AshleyRoll/arquebus>

## References
* [Producer–consumer problem](https://en.wikipedia.org/wiki/Producer%E2%80%93consumer_problem)
* [Readers–writers problem](https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem)
* [Implementing Go-style Channels in C++ from scratch](https://jyotinder.substack.com/p/implementing-go-channels-in-cpp)
* <https://github.com/JyotinderSingh/CppChan/>
* [C++ Channel: A thread-safe container for sharing data between threads](https://blog.andreiavram.ro/cpp-channel-thread-safe-container-share-data-threads/)
* <https://github.com/andreiavrammsd/cpp-channel/>
* [User API & C++ Implementation of a Multi Producer, Multi Consumer, Lock Free, Atomic Queue - CppCon](https://www.youtube.com/watch?v=bjz_bMNNWRk), [code](https://github.com/erez-strauss/lockfree_mpmc_queue)

