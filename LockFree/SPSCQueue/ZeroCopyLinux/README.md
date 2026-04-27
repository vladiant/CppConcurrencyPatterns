# Linux zero-copy ring buffer 

Uses double-mapped virtual memory trick — the buffer is mapped twice contiguously so that any read/write spanning the boundary works without copying or wrapping logic.

---

## How It Works

The key insight is to `mmap` the same physical memory **twice, back-to-back** in virtual address space:

```
Virtual Address Space:
┌─────────────────┬─────────────────┐
│   Mirror Copy   │  Mirror Copy 2  │
│  [0 ... N-1]    │  [0 ... N-1]    │
└─────────────────┴─────────────────┘
         ↕ both map to same physical pages ↕
```

This means a write of size `S` at any position never needs to wrap — you can always `memcpy` in one shot.

---

## Key Design Decisions

| Decision | Detail |
|---|---|
| **Double-mmap trick** | Two `MAP_SHARED` + `MAP_FIXED` mappings of the same `shm` fd, back-to-back. Any pointer into the first half automatically continues into the second half for free. |
| **Power-of-two capacity** | Rounded up to page size (which is itself always a power-of-two). Enables cheap `& (capacity - 1)` masking instead of `%`. |
| **Ever-incrementing cursors** | `read_pos_` / `write_pos_` never wrap; only the *masked* value indexes into the buffer. This avoids the ambiguous full/empty problem. |
| **False-sharing prevention** | The two atomic cursors are placed on **separate 64-byte cache lines** (`alignas(64)`), preventing producer/consumer cache-line ping-pong. |
| **SPSC lock-free** | Uses `std::atomic` with `acquire`/`release` ordering — no mutex, no CAS loop, safe for one producer and one consumer thread. |

### Build & Run

```bash
cmake ..
make
./ring_buffer
# [basic] received: Hello, zero-copy world!
# [wrap] cross-boundary zero-copy read/write OK
# [spsc] 100000 messages exchanged OK
# All examples passed.
```
