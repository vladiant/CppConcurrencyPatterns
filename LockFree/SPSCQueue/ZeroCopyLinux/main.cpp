#include <cassert>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#include "ring_buffer.hpp"

// ─── Example 1: basic write / read ───────────────────────────────────────────
static void example_basic() {
  RingBuffer rb(4096);  // one page

  const char msg[] = "Hello, zero-copy world!";
  assert(rb.write(msg, sizeof(msg)));

  char out[sizeof(msg)]{};
  assert(rb.read(out, sizeof(out)));

  std::cout << "[basic] received: " << out << "\n";
  assert(rb.empty());
}

// ─── Example 2: zero-copy write across the physical wrap boundary
// ─────────────
static void example_wrap_around() {
  RingBuffer rb(4096);

  // Fill all but 16 bytes to position write cursor near the end
  std::vector<uint8_t> filler(rb.capacity() - 16, 0xAB);
  assert(rb.write(filler.data(), filler.size()));

  // Consume everything so the read cursor is also near the end
  std::vector<uint8_t> discard(filler.size());
  assert(rb.read(discard.data(), discard.size()));

  // Now write 32 bytes — 16 fit before the physical wrap, 16 after.
  // Thanks to the mirror mapping this is ONE contiguous memcpy.
  uint8_t payload[32];
  for (int i = 0; i < 32; ++i) payload[i] = static_cast<uint8_t>(i);

  uint8_t* dst = rb.write_ptr(32);
  assert(dst != nullptr);
  ::memcpy(dst, payload, 32);
  rb.commit_write(32);

  // Read it back — again one contiguous pointer, no wrap handling needed
  const uint8_t* src = rb.read_ptr(32);
  assert(src != nullptr);
  for (int i = 0; i < 32; ++i) assert(src[i] == static_cast<uint8_t>(i));
  rb.commit_read(32);

  std::cout << "[wrap] cross-boundary zero-copy read/write OK\n";
}

// ─── Example 3: SPSC producer / consumer threads ─────────────────────────────
static void example_spsc() {
  constexpr int MESSAGES = 100'000;
  constexpr size_t MSG_SIZE = 64;

  RingBuffer rb(1 << 16);  // 64 KiB

  std::thread producer([&] {
    uint8_t buf[MSG_SIZE];
    for (int i = 0; i < MESSAGES; ++i) {
      ::memset(buf, static_cast<uint8_t>(i & 0xFF), MSG_SIZE);
      while (!rb.write(buf, MSG_SIZE));  // spin until space is available
    }
  });

  std::thread consumer([&] {
    uint8_t buf[MSG_SIZE];
    for (int i = 0; i < MESSAGES; ++i) {
      while (!rb.read(buf, MSG_SIZE));  // spin until data is available
      // Verify payload
      for (size_t b = 0; b < MSG_SIZE; ++b)
        assert(buf[b] == static_cast<uint8_t>(i & 0xFF));
    }
  });

  producer.join();
  consumer.join();

  std::cout << "[spsc] " << MESSAGES << " messages exchanged OK\n";
}

int main() {
  example_basic();
  example_wrap_around();
  example_spsc();
  std::cout << "All examples passed.\n";
}
