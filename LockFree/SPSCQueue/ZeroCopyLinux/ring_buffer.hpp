#pragma once

#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

/// Zero-copy ring buffer using the double-mapped virtual memory trick.
/// The underlying physical pages are mapped twice consecutively in virtual
/// address space, so reads and writes never need to wrap around manually.
///
/// Thread-safety: Single producer / single consumer (SPSC) lock-free.
class RingBuffer {
 public:
  /// @param capacity Desired capacity in bytes. Will be rounded up to a
  ///                 multiple of the system page size.
  explicit RingBuffer(std::size_t capacity) {
    const std::size_t page_size = static_cast<std::size_t>(::getpagesize());

    // Round up to nearest page boundary
    capacity_ = (capacity + page_size - 1) & ~(page_size - 1);

    // Create an anonymous file in shared memory
    const std::string path = "/dev/shm/ringbuf_XXXXXX";
    std::string tmp(path);
    int fd = ::mkstemp(tmp.data());
    if (fd < 0)
      throw std::runtime_error(std::string("mkstemp failed: ") +
                               ::strerror(errno));

    // Unlink immediately — the fd keeps it alive
    ::unlink(tmp.c_str());

    if (::ftruncate(fd, static_cast<off_t>(capacity_)) < 0) {
      ::close(fd);
      throw std::runtime_error(std::string("ftruncate failed: ") +
                               ::strerror(errno));
    }

    // Reserve 2× capacity of virtual address space
    void* addr = ::mmap(nullptr, capacity_ * 2, PROT_NONE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
      ::close(fd);
      throw std::runtime_error(std::string("mmap (reserve) failed: ") +
                               ::strerror(errno));
    }

    base_ = static_cast<uint8_t*>(addr);

    // Map the shared memory into the first half
    void* first = ::mmap(base_, capacity_, PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_FIXED, fd, 0);
    if (first == MAP_FAILED) {
      ::munmap(base_, capacity_ * 2);
      ::close(fd);
      throw std::runtime_error(std::string("mmap (first) failed: ") +
                               ::strerror(errno));
    }

    // Map the same shared memory into the second half (the mirror)
    void* second = ::mmap(base_ + capacity_, capacity_, PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_FIXED, fd, 0);
    if (second == MAP_FAILED) {
      ::munmap(base_, capacity_ * 2);
      ::close(fd);
      throw std::runtime_error(std::string("mmap (mirror) failed: ") +
                               ::strerror(errno));
    }

    ::close(fd);  // fd no longer needed after mapping

    read_pos_.store(0, std::memory_order_relaxed);
    write_pos_.store(0, std::memory_order_relaxed);
  }

  ~RingBuffer() { ::munmap(base_, capacity_ * 2); }

  // Non-copyable, non-movable (owns raw memory mapping)
  RingBuffer(const RingBuffer&) = delete;
  RingBuffer& operator=(const RingBuffer&) = delete;
  RingBuffer(RingBuffer&&) = delete;
  RingBuffer& operator=(RingBuffer&&) = delete;

  /// Total capacity of the buffer in bytes.
  [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

  /// Bytes available to read.
  [[nodiscard]] std::size_t size() const noexcept {
    const std::size_t w = write_pos_.load(std::memory_order_acquire);
    const std::size_t r = read_pos_.load(std::memory_order_acquire);
    return w - r;  // unsigned wrap handles the modular arithmetic
  }

  /// Free space available for writing.
  [[nodiscard]] std::size_t free_space() const noexcept {
    return capacity_ - size();
  }

  /// Returns true if there is no data to read.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  /// Returns true if no more data can be written.
  [[nodiscard]] bool full() const noexcept { return size() == capacity_; }

  // -------------------------------------------------------------------------
  // Zero-copy write: obtain a writable pointer, then commit
  // -------------------------------------------------------------------------

  /// Returns a pointer into the buffer where up to `len` bytes may be
  /// written directly. Call commit_write() afterwards with the actual
  /// number of bytes written. Returns nullptr if there is not enough space.
  [[nodiscard]] uint8_t* write_ptr(std::size_t len) noexcept {
    if (len > free_space()) return nullptr;
    const std::size_t w = write_pos_.load(std::memory_order_relaxed);
    return base_ + (w & (capacity_ - 1));
    //                   ^^^^^^^^^^^^^^^^^^^
    // Because capacity_ is a power-of-two page multiple, we can use a
    // fast bitmask instead of modulo.
  }

  /// Advances the write pointer by `len` bytes after a direct write.
  void commit_write(std::size_t len) noexcept {
    write_pos_.fetch_add(len, std::memory_order_release);
  }

  // -------------------------------------------------------------------------
  // Zero-copy read: obtain a readable pointer, then consume
  // -------------------------------------------------------------------------

  /// Returns a pointer to the next `len` bytes available for reading.
  /// Because of the double-mapping, the data is always contiguous even if
  /// it wraps around the physical buffer boundary. Returns nullptr if there
  /// are fewer than `len` bytes available.
  [[nodiscard]] const uint8_t* read_ptr(std::size_t len) const noexcept {
    if (len > size()) return nullptr;
    const std::size_t r = read_pos_.load(std::memory_order_relaxed);
    return base_ + (r & (capacity_ - 1));
  }

  /// Advances the read pointer by `len` bytes after consuming data.
  void commit_read(std::size_t len) noexcept {
    read_pos_.fetch_add(len, std::memory_order_release);
  }

  // -------------------------------------------------------------------------
  // Convenience helpers (do use a memcpy internally)
  // -------------------------------------------------------------------------

  /// Copy `len` bytes from `src` into the ring buffer.
  /// Returns false if there is not enough free space.
  bool write(const void* src, std::size_t len) noexcept {
    uint8_t* dst = write_ptr(len);
    if (!dst) return false;
    ::memcpy(dst, src, len);
    commit_write(len);
    return true;
  }

  /// Copy `len` bytes out of the ring buffer into `dst`.
  /// Returns false if there is not enough data.
  bool read(void* dst, std::size_t len) noexcept {
    const uint8_t* src = read_ptr(len);
    if (!src) return false;
    ::memcpy(dst, src, len);
    commit_read(len);
    return true;
  }

 private:
  uint8_t* base_ = nullptr;
  std::size_t capacity_ = 0;

  alignas(64) std::atomic<std::size_t> write_pos_{0};  // producer cache line
  alignas(64) std::atomic<std::size_t> read_pos_{0};   // consumer cache line
};
