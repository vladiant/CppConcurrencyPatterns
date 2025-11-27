## Source
* <https://github.com/bowtoyourlord/MPSCQueue>

## Test Coverage
* SingleThreadedBasic - Sanity check for basic functionality
* CapacityLimit - Verifies the queue correctly enforces capacity limits
* MultipleProducersSingleConsumer - Core correctness test ensuring no lost or duplicate messages
* HighContentionStressTest - Tests behavior under heavy concurrent load with 8 producers
* FIFOOrderingPerProducer - Verifies that each producer's messages maintain FIFO ordering
* FullQueueBehavior - Tests that producers correctly fail when queue is full
* AlternatingPushPop - Tests interleaved push/pop operations
* VariableRateProducers - Tests with producers operating at different speeds

## Key Testing Strategies
* No duplicates: Uses sets to detect duplicate values
* No lost messages: Counts all messages and verifies completeness
* FIFO per producer: Encodes producer ID and sequence number to verify ordering
* High contention: Uses many threads starting simultaneously
* Timeout protection: Prevents tests from hanging indefinitely
