#include "mpsc_queue.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <thread>
#include <vector>

// Simple test framework
int total_tests = 0;
int passed_tests = 0;
int failed_tests = 0;

#define TEST_ASSERT(condition, message)                              \
  do {                                                               \
    if (!(condition)) {                                              \
      std::cout << "  ✗ ASSERTION FAILED: " << message << std::endl; \
      std::cout << "    at line " << __LINE__ << std::endl;          \
      return false;                                                  \
    }                                                                \
  } while (0)

#define RUN_TEST(test_func)                                               \
  do {                                                                    \
    total_tests++;                                                        \
    std::cout << "\n[TEST " << total_tests << "] " << #test_func << "..." \
              << std::endl;                                               \
    if (test_func()) {                                                    \
      passed_tests++;                                                     \
      std::cout << "  ✓ PASSED" << std::endl;                             \
    } else {                                                              \
      failed_tests++;                                                     \
      std::cout << "  ✗ FAILED" << std::endl;                             \
    }                                                                     \
  } while (0)

// Basic single-threaded sanity test
bool test_single_threaded_basic() {
  MPSCQueue<int> queue;

  TEST_ASSERT(queue.push(1), "Failed to push 1");
  TEST_ASSERT(queue.push(2), "Failed to push 2");
  TEST_ASSERT(queue.push(3), "Failed to push 3");

  auto val1 = queue.pop();
  TEST_ASSERT(val1.has_value() && val1.value() == 1, "Expected 1");

  auto val2 = queue.pop();
  TEST_ASSERT(val2.has_value() && val2.value() == 2, "Expected 2");

  auto val3 = queue.pop();
  TEST_ASSERT(val3.has_value() && val3.value() == 3, "Expected 3");

  TEST_ASSERT(!queue.pop().has_value(), "Expected empty queue");

  return true;
}

// Test that queue correctly reports full when at capacity
bool test_capacity_limit() {
  MPSCQueue<int> queue;

  for (size_t i = 0; i < MPSC_QUEUE_CAPACITY; ++i) {
    if (!queue.push(i)) {
      std::cout << "  Failed to push at index " << i << std::endl;
      return false;
    }
  }

  TEST_ASSERT(!queue.push(9999), "Queue should be full");

  return true;
}

// Test multiple producers, single consumer - correctness
bool test_multiple_producers_single_consumer() {
  MPSCQueue<int> queue;
  const int NUM_PRODUCERS = 4;
  const int ITEMS_PER_PRODUCER = 1000;
  const int TOTAL_ITEMS = NUM_PRODUCERS * ITEMS_PER_PRODUCER;

  std::atomic<bool> start{false};
  std::vector<std::thread> producers;

  // Launch producers
  for (int p = 0; p < NUM_PRODUCERS; ++p) {
    producers.emplace_back([&, p]() {
      while (!start.load(std::memory_order_acquire)) {
        std::this_thread::yield();
      }

      for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
        int value = p * ITEMS_PER_PRODUCER + i;
        while (!queue.push(value)) {
          std::this_thread::yield();
        }
      }
    });
  }

  // Start all producers simultaneously
  start.store(true, std::memory_order_release);

  // Consumer thread
  std::set<int> received;
  int consumed = 0;

  while (consumed < TOTAL_ITEMS) {
    auto item = queue.pop();
    if (item.has_value()) {
      received.insert(item.value());
      consumed++;
    } else {
      std::this_thread::yield();
    }
  }

  // Wait for all producers to finish
  for (auto& t : producers) {
    t.join();
  }

  // Verify all items received exactly once
  TEST_ASSERT(received.size() == TOTAL_ITEMS, "Not all items received");

  for (int i = 0; i < TOTAL_ITEMS; ++i) {
    if (received.count(i) != 1) {
      std::cout << "  Missing or duplicate item: " << i << std::endl;
      return false;
    }
  }

  return true;
}

// Test high contention scenario
bool test_high_contention_stress() {
  MPSCQueue<int> queue;
  const int NUM_PRODUCERS = 8;
  const int ITEMS_PER_PRODUCER = 5000;
  const int TOTAL_ITEMS = NUM_PRODUCERS * ITEMS_PER_PRODUCER;

  std::atomic<int> push_failures{0};
  std::atomic<bool> start{false};
  std::vector<std::thread> producers;

  // Launch producers with high contention
  for (int p = 0; p < NUM_PRODUCERS; ++p) {
    producers.emplace_back([&, p]() {
      while (!start.load(std::memory_order_acquire)) {
      }

      for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
        int value = p * ITEMS_PER_PRODUCER + i;
        int retries = 0;
        while (!queue.push(value)) {
          retries++;
          if (retries > 1000) {
            push_failures.fetch_add(1, std::memory_order_relaxed);
            break;
          }
          std::this_thread::yield();
        }
      }
    });
  }

  start.store(true, std::memory_order_release);

  // Consumer
  std::vector<int> received;
  received.reserve(TOTAL_ITEMS);

  auto start_time = std::chrono::steady_clock::now();
  auto timeout = std::chrono::seconds(10);

  while (received.size() <
         TOTAL_ITEMS - push_failures.load(std::memory_order_relaxed)) {
    auto item = queue.pop();
    if (item.has_value()) {
      received.push_back(item.value());
    }

    if (std::chrono::steady_clock::now() - start_time > timeout) {
      break;
    }
  }

  for (auto& t : producers) {
    t.join();
  }

  // Verify no duplicates
  std::sort(received.begin(), received.end());
  auto last = std::unique(received.begin(), received.end());
  TEST_ASSERT(last == received.end(), "Duplicate items detected!");

  std::cout << "  Consumed " << received.size() << " items, "
            << push_failures.load() << " push failures" << std::endl;

  return true;
}

// Test FIFO ordering per producer
bool test_fifo_ordering_per_producer() {
  MPSCQueue<int> queue;
  const int NUM_PRODUCERS = 4;
  const int ITEMS_PER_PRODUCER = 500;

  std::vector<std::thread> producers;
  std::atomic<bool> start{false};

  // Each producer sends sequential items
  for (int p = 0; p < NUM_PRODUCERS; ++p) {
    producers.emplace_back([&, p]() {
      while (!start.load(std::memory_order_acquire)) {
      }

      for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
        // Encode producer ID in high bits, sequence in low bits
        int value = (p << 20) | i;
        while (!queue.push(value)) {
          std::this_thread::yield();
        }
      }
    });
  }

  start.store(true, std::memory_order_release);

  // Track last sequence number per producer
  std::vector<int> last_seq(NUM_PRODUCERS, -1);
  int total_consumed = 0;
  int expected_total = NUM_PRODUCERS * ITEMS_PER_PRODUCER;

  while (total_consumed < expected_total) {
    auto item = queue.pop();
    if (item.has_value()) {
      int value = item.value();
      int producer_id = value >> 20;
      int sequence = value & 0xFFFFF;

      // Verify FIFO per producer
      if (sequence <= last_seq[producer_id]) {
        std::cout << "  FIFO violation for producer " << producer_id
                  << ": got sequence " << sequence << " after "
                  << last_seq[producer_id] << std::endl;
        return false;
      }
      last_seq[producer_id] = sequence;
      total_consumed++;
    }
  }

  for (auto& t : producers) {
    t.join();
  }

  // Verify all sequences complete
  for (int p = 0; p < NUM_PRODUCERS; ++p) {
    if (last_seq[p] != ITEMS_PER_PRODUCER - 1) {
      std::cout << "  Producer " << p
                << " incomplete: last_seq = " << last_seq[p] << std::endl;
      return false;
    }
  }

  return true;
}

// Test queue behavior when full
bool test_full_queue_behavior() {
  MPSCQueue<int> queue;
  const int NUM_PRODUCERS = 4;
  std::atomic<int> successful_pushes{0};
  std::atomic<bool> start{false};

  // Fill the queue first
  for (size_t i = 0; i < MPSC_QUEUE_CAPACITY; ++i) {
    if (!queue.push(i)) {
      std::cout << "  Failed to fill queue at index " << i << std::endl;
      return false;
    }
  }

  // Try to push more from multiple producers
  std::vector<std::thread> producers;
  for (int p = 0; p < NUM_PRODUCERS; ++p) {
    producers.emplace_back([&]() {
      while (!start.load(std::memory_order_acquire)) {
      }

      for (int i = 0; i < 100; ++i) {
        if (queue.push(999)) {
          successful_pushes.fetch_add(1, std::memory_order_relaxed);
        }
      }
    });
  }

  start.store(true, std::memory_order_release);

  for (auto& t : producers) {
    t.join();
  }

  // All pushes should have failed (queue was full)
  TEST_ASSERT(successful_pushes.load() == 0,
              "Pushes should fail when queue is full");

  return true;
}

// Test alternating push/pop pattern
bool test_alternating_push_pop() {
  MPSCQueue<int> queue;
  const int NUM_PRODUCERS = 3;
  const int OPERATIONS = 1000;

  std::atomic<bool> stop{false};
  std::atomic<int> total_pushed{0};
  std::vector<std::thread> producers;

  for (int p = 0; p < NUM_PRODUCERS; ++p) {
    producers.emplace_back([&, p]() {
      for (int i = 0; i < OPERATIONS; ++i) {
        int value = p * OPERATIONS + i;
        while (!queue.push(value) && !stop.load()) {
          std::this_thread::yield();
        }
        total_pushed.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  // Consumer alternates between consuming and yielding
  int consumed = 0;
  int expected = NUM_PRODUCERS * OPERATIONS;

  while (consumed < expected) {
    auto item = queue.pop();
    if (item.has_value()) {
      consumed++;
    }
    if (consumed % 10 == 0) {
      std::this_thread::yield();
    }
  }

  for (auto& t : producers) {
    t.join();
  }

  TEST_ASSERT(consumed == expected, "Not all items consumed");

  return true;
}

// Test with variable-rate producers
bool test_variable_rate_producers() {
  MPSCQueue<int> queue;
  const int NUM_PRODUCERS = 4;
  const int ITEMS_PER_PRODUCER = 500;

  std::vector<std::thread> producers;
  std::atomic<bool> start{false};
  std::mt19937 rng(42);

  for (int p = 0; p < NUM_PRODUCERS; ++p) {
    producers.emplace_back([&, p, rng]() mutable {
      while (!start.load(std::memory_order_acquire)) {
      }

      std::uniform_int_distribution<int> dist(0, 10);
      for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
        int value = p * ITEMS_PER_PRODUCER + i;
        while (!queue.push(value)) {
          std::this_thread::yield();
        }

        // Random delay
        if (dist(rng) < 2) {
          std::this_thread::yield();
        }
      }
    });
  }

  start.store(true, std::memory_order_release);

  std::set<int> received;
  int expected = NUM_PRODUCERS * ITEMS_PER_PRODUCER;

  while (received.size() < expected) {
    auto item = queue.pop();
    if (item.has_value()) {
      if (received.count(item.value()) != 0) {
        std::cout << "  Duplicate value: " << item.value() << std::endl;
        return false;
      }
      received.insert(item.value());
    }
  }

  for (auto& t : producers) {
    t.join();
  }

  TEST_ASSERT(received.size() == expected, "Not all items received");

  return true;
}

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "  MPSC Queue Concurrency Tests" << std::endl;
  std::cout << "========================================" << std::endl;

  RUN_TEST(test_single_threaded_basic);
  RUN_TEST(test_capacity_limit);
  RUN_TEST(test_multiple_producers_single_consumer);
  RUN_TEST(test_high_contention_stress);
  RUN_TEST(test_fifo_ordering_per_producer);
  RUN_TEST(test_full_queue_behavior);
  RUN_TEST(test_alternating_push_pop);
  RUN_TEST(test_variable_rate_producers);

  std::cout << "\n========================================" << std::endl;
  std::cout << "  Test Summary" << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << "Total tests:  " << total_tests << std::endl;
  std::cout << "Passed:       " << passed_tests << " ✓" << std::endl;
  std::cout << "Failed:       " << failed_tests << " ✗" << std::endl;
  std::cout << "========================================" << std::endl;

  return (failed_tests == 0) ? 0 : 1;
}
