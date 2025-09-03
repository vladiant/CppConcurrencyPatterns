// Copyright 2018 Kaspar Daugaard. For educational purposes only.
// See http://daugaard.org/blog/writing-a-fast-and-versatile-spsc-ring-buffer

// Requires a semaphore implementation, e.g. Jeff Preshing's:
// https://github.com/preshing/cpp11-on-multicore/blob/master/common/sema.h
#include <algorithm>
#include <atomic>

#include "sema.h"

#define CACHE_LINE_SIZE 64

class RingBuffer {
 public:
  // Allocate buffer space for writing.
  void* PrepareWrite(size_t size, size_t alignment);

  // Publish written data.
  void FinishWrite();

  // Write an element to the buffer.
  template <typename T>
  void Write(const T& value) {
    void* dest = PrepareWrite(sizeof(T), alignof(T));
    new (dest) T(value);
  }

  // Write an array of elements to the buffer.
  template <typename T>
  void WriteArray(const T* values, size_t count) {
    void* dest = PrepareWrite(sizeof(T) * count, alignof(T));
    for (size_t i = 0; i < count; i++)
      new (static_cast<T*>(dest) + i) T(values[i]);
  }

  // Get read pointer. Size and alignment should match written data.
  void* PrepareRead(size_t size, size_t alignment);

  // Finish and make buffer space available to writer.
  void FinishRead();

  // Read an element from the buffer.
  template <typename T>
  const T& Read() {
    void* src = PrepareRead(sizeof(T), alignof(T));
    return *static_cast<T*>(src);
  }

  // Read an array of elements from the buffer.
  template <typename T>
  const T* ReadArray(size_t count) {
    void* src = PrepareRead(sizeof(T) * count, alignof(T));
    return static_cast<T*>(src);
  }

  // Initialize. Buffer must have required alignment. Size must be a power of
  // two.
  void Initialize(void* buffer, size_t size) {
    Reset();
    m_Reader.buffer = m_Writer.buffer = static_cast<char*>(buffer);
    m_Reader.size = m_Writer.size = m_Writer.end = size;
  }

  void Reset() {
    m_Reader = m_Writer = LocalState();
    m_ReaderShared.pos = m_WriterShared.pos = 0;
  }

 private:
  static size_t Align(size_t pos, size_t alignment) {
#ifdef RINGBUFFER_DO_NOT_ALIGN
    alignment = 1;
#endif
    return (pos + alignment - 1) & ~(alignment - 1);
  }

  void GetBufferSpaceToWriteTo(size_t& pos, size_t& end);
  void GetBufferSpaceToReadFrom(size_t& pos, size_t& end);

  // Writer and reader's local state.
  struct alignas(CACHE_LINE_SIZE) LocalState {
    LocalState() : buffer(nullptr), pos(0), end(0), base(0), size(0) {}
    char* buffer;
    size_t pos;
    size_t end;
    size_t base;
    size_t size;
  };

  LocalState m_Writer;
  LocalState m_Reader;

  // Writer and reader's shared state.
  struct alignas(CACHE_LINE_SIZE) SharedState {
    std::atomic<size_t> pos;
    std::atomic<bool> shouldSignal;
    Semaphore semaphore;
  };

  SharedState m_WriterShared;
  SharedState m_ReaderShared;
};

void* RingBuffer::PrepareWrite(size_t size, size_t alignment) {
  size_t pos = Align(m_Writer.pos, alignment);
  size_t end = pos + size;
  if (end > m_Writer.end) GetBufferSpaceToWriteTo(pos, end);
  m_Writer.pos = end;
  return m_Writer.buffer + pos;
}

void RingBuffer::FinishWrite() {
  m_WriterShared.pos.store(m_Writer.base + m_Writer.pos,
                           std::memory_order_release);
  if (m_WriterShared.shouldSignal.exchange(false))
    m_ReaderShared.semaphore.signal();
}

void* RingBuffer::PrepareRead(size_t size, size_t alignment) {
  size_t pos = Align(m_Reader.pos, alignment);
  size_t end = pos + size;
  if (end > m_Reader.end) GetBufferSpaceToReadFrom(pos, end);
  m_Reader.pos = end;
  return m_Reader.buffer + pos;
}

void RingBuffer::FinishRead() {
  m_ReaderShared.pos.store(m_Reader.base + m_Reader.pos,
                           std::memory_order_release);
  if (m_ReaderShared.shouldSignal.exchange(false))
    m_WriterShared.semaphore.signal();
}

void RingBuffer::GetBufferSpaceToWriteTo(size_t& pos, size_t& end) {
  if (end > m_Writer.size) {
    end -= pos;
    pos = 0;
    m_Writer.base += m_Writer.size;
  }
  for (;;) {
    size_t readerPos = m_ReaderShared.pos.load(std::memory_order_acquire);
    size_t available = readerPos - m_Writer.base + m_Writer.size;
    // Signed comparison (available can be negative)
    if (static_cast<ptrdiff_t>(available) >= static_cast<ptrdiff_t>(end)) {
      m_Writer.end = std::min(available, m_Writer.size);
      break;
    }
    m_ReaderShared.shouldSignal.store(true);
    if (readerPos != m_ReaderShared.pos.load(std::memory_order_relaxed)) {
      // Position changed after we requested signal, try to cancel
      if (m_ReaderShared.shouldSignal.exchange(false))
        continue;  // Request successfully cancelled
    }
    m_WriterShared.semaphore.wait();
  }
}

void RingBuffer::GetBufferSpaceToReadFrom(size_t& pos, size_t& end) {
  if (end > m_Reader.size) {
    end -= pos;
    pos = 0;
    m_Reader.base += m_Reader.size;
  }
  for (;;) {
    size_t writerPos = m_WriterShared.pos.load(std::memory_order_acquire);
    size_t available = writerPos - m_Reader.base;
    // Signed comparison (available can be negative)
    if (static_cast<ptrdiff_t>(available) >= static_cast<ptrdiff_t>(end)) {
      m_Reader.end = std::min(available, m_Reader.size);
      break;
    }
    m_WriterShared.shouldSignal.store(true);
    if (writerPos != m_WriterShared.pos.load(std::memory_order_relaxed)) {
      // Position changed after we requested signal, try to cancel
      if (m_WriterShared.shouldSignal.exchange(false))
        continue;  // Request successfully cancelled
    }
    m_ReaderShared.semaphore.wait();
  }
}
