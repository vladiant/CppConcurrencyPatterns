## Producer/Consumer pattern
The Producer/Consumer pattern is used to decouple processes that produce and consume data at different rates. The Producer/Consumer pattern's parallel loops are broken down into two categories; those that produce data, and those that consume the data produced.

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

## References
* [Producer–consumer problem](https://en.wikipedia.org/wiki/Producer%E2%80%93consumer_problem)
* [Readers–writers problem](https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem)

