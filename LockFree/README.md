# Lock Free Data Structures

## Lock free queue types
* Consumers - single, multi
* Producers - single, multi
* Pop on empty - return false, return sentinel
* Push when full - return false, overwrite
* Favour - readers, writers

## Examples
* [SPSC Lock-free, Wait-free Fifo from the Ground Up](https://github.com/CharlesFrasch/cppcon2023)
* <https://github.com/erez-strauss/lockfree_mpmc_queue>
* <https://github.com/max0x7ba/atomic_queue>
* [Многопоточность в современном C++: Lock-Free программирование, Memory Ordering и Atomics](https://habr.com/ru/articles/963818/)
* [Boost.Lockfree](https://theboostcpplibraries.com/boost.lockfree)
