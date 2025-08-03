# Phase 3: Buffer Management - Detailed Component Specification

**Date:** October 12, 2025
**Status:** 🔲 Planned (Phase 1 & 2 Complete)
**Estimated Effort:** 3-4 days, ~1500 lines of code, ~25 tests

---

## Overview

The buffer management layer provides an in-memory cache of disk blocks (pages) to reduce I/O operations. It manages a fixed-size pool of buffers with pinning semantics and eviction policies.

### Key Responsibilities
1. **Caching:** Keep frequently accessed pages in memory
2. **Pin/Unpin:** Prevent eviction of pages in use
3. **WAL Compliance:** Flush log before flushing dirty pages
4. **Eviction:** Choose victims when pool is full
5. **Concurrency:** Thread-safe buffer allocation

---

## Component 1: Buffer

**Files to Create:**
- `include/buffer/buffer.hpp`
- `src/buffer/buffer.cpp`

### Purpose
Wrapper around `Page` that adds:
- Association with a `BlockId`
- Pin/unpin reference counting
- Modification tracking (dirty flag, transaction number, LSN)
- WAL-compliant flush semantics

### API Specification

```cpp
#ifndef BUFFER_HPP
#define BUFFER_HPP

#include "file/page.hpp"
#include "file/blockid.hpp"
#include "file/filemgr.hpp"
#include "log/logmgr.hpp"
#include <memory>
#include <optional>

namespace buffer {

/**
 * Buffer wraps a Page with pinning and modification tracking.
 *
 * A buffer can be in one of three states:
 * 1. Unpinned and unassigned (blk = None)
 * 2. Unpinned and assigned to a block
 * 3. Pinned (assigned to a block, pins > 0)
 *
 * Corresponds to Buffer in Rust (NMDB2/src/buffer/buffer.rs)
 */
class Buffer {
public:
    /**
     * Creates a new buffer.
     *
     * @param fm the file manager
     * @param lm the log manager
     */
    Buffer(std::shared_ptr<file::FileMgr> fm,
           std::shared_ptr<log::LogMgr> lm);

    /**
     * Returns a reference to the buffer's page contents.
     * Allows reading and writing to the page.
     *
     * @return reference to the page
     */
    file::Page& contents();

    /**
     * Returns the block this buffer is assigned to.
     *
     * @return the block, or std::nullopt if unassigned
     */
    const std::optional<file::BlockId>& block() const;

    /**
     * Marks the buffer as modified by the specified transaction.
     *
     * @param txnum the modifying transaction number
     * @param lsn the log sequence number (optional)
     */
    void set_modified(size_t txnum, std::optional<size_t> lsn);

    /**
     * Returns whether the buffer is currently pinned.
     *
     * @return true if pins > 0
     */
    bool is_pinned() const;

    /**
     * Returns the transaction number that modified this buffer.
     *
     * @return the transaction number, or std::nullopt if not modified
     */
    std::optional<size_t> modifying_tx() const;

    // Package-private methods (used only by BufferMgr)
    // In C++, we'll make BufferMgr a friend class

private:
    friend class BufferMgr;

    /**
     * Assigns this buffer to a block.
     * Flushes the previous block if dirty, then reads the new block.
     * Resets pins to 0.
     *
     * @param blk the block to assign
     */
    void assign_to_block(const file::BlockId& blk);

    /**
     * Flushes the buffer to disk if it has been modified.
     * Follows WAL: flushes log first if lsn is set.
     * Resets txnum to None after flushing.
     */
    void flush();

    /**
     * Increments the pin count.
     */
    void pin();

    /**
     * Decrements the pin count.
     */
    void unpin();

private:
    std::shared_ptr<file::FileMgr> fm_;
    std::shared_ptr<log::LogMgr> lm_;
    file::Page contents_;
    std::optional<file::BlockId> blk_;
    int32_t pins_;
    std::optional<size_t> txnum_;
    std::optional<size_t> lsn_;
};

} // namespace buffer

#endif // BUFFER_HPP
```

### Implementation Notes

#### Constructor
```cpp
Buffer::Buffer(std::shared_ptr<file::FileMgr> fm,
               std::shared_ptr<log::LogMgr> lm)
    : fm_(fm),
      lm_(lm),
      contents_(fm->block_size()),
      blk_(std::nullopt),
      pins_(0),
      txnum_(std::nullopt),
      lsn_(std::nullopt) {
}
```

#### Key Methods

**`assign_to_block()`**
```cpp
void Buffer::assign_to_block(const file::BlockId& blk) {
    flush();  // Flush old block if dirty
    blk_ = blk;
    fm_->read(blk, contents_);
    pins_ = 0;
}
```

**`flush()`** (WAL-compliant)
```cpp
void Buffer::flush() {
    if (txnum_.has_value()) {
        // Flush log first (WAL)
        if (lsn_.has_value()) {
            lm_->flush(lsn_.value());
        }
        // Then flush data page
        if (blk_.has_value()) {
            fm_->write(blk_.value(), contents_);
        }
        txnum_ = std::nullopt;  // Mark as clean
    }
}
```

**`set_modified()`**
```cpp
void Buffer::set_modified(size_t txnum, std::optional<size_t> lsn) {
    txnum_ = txnum;
    if (lsn.has_value()) {
        lsn_ = lsn;
    }
}
```

### Test Coverage (10 tests)

**File:** `tests/test_buffer.cpp`

```cpp
TEST(BufferTest, InitialState) {
    // New buffer is unpinned and unassigned
    Buffer buf(fm, lm);
    EXPECT_FALSE(buf.is_pinned());
    EXPECT_FALSE(buf.block().has_value());
    EXPECT_FALSE(buf.modifying_tx().has_value());
}

TEST(BufferTest, AssignToBlock) {
    // Assign buffer to a block and verify it reads the block
}

TEST(BufferTest, PinUnpin) {
    // Pin increments count, unpin decrements
}

TEST(BufferTest, SetModified) {
    // Set modified updates txnum and lsn
}

TEST(BufferTest, FlushDirtyBuffer) {
    // Flush writes dirty buffer to disk
}

TEST(BufferTest, FlushClearsModification) {
    // After flush, txnum is None
}

TEST(BufferTest, FlushUnmodifiedBufferIsNoop) {
    // Flush on clean buffer does nothing
}

TEST(BufferTest, FlushRespectsWAL) {
    // Log is flushed before data page
}

TEST(BufferTest, ReassignFlushesOldBlock) {
    // assign_to_block flushes previous dirty block
}

TEST(BufferTest, MultiplePins) {
    // Pin count increments correctly
}
```

---

## Component 2: BufferMgr

**Files to Create:**
- `include/buffer/buffermgr.hpp`
- `src/buffer/buffermgr.cpp`

### Purpose
Manages a pool of buffers with:
- Pin/unpin operations
- Buffer allocation and eviction
- Waiting strategy when pool is full

### API Specification

```cpp
#ifndef BUFFERMGR_HPP
#define BUFFERMGR_HPP

#include "buffer/buffer.hpp"
#include "file/blockid.hpp"
#include "file/filemgr.hpp"
#include "log/logmgr.hpp"
#include <memory>
#include <vector>
#include <chrono>
#include <stdexcept>

namespace buffer {

/**
 * Exception thrown when buffer pool is full and timeout expires.
 */
class BufferAbortException : public std::runtime_error {
public:
    BufferAbortException() : std::runtime_error("Buffer abort: pool exhausted") {}
};

/**
 * BufferMgr manages a fixed-size pool of buffers.
 *
 * Pin/Unpin Protocol:
 * - pin(blk) returns a buffer index, increments pin count
 * - unpin(idx) decrements pin count, makes buffer available
 * - Buffers with pins > 0 cannot be evicted
 *
 * Eviction Policy:
 * - Simple: first unpinned buffer found (naive LRU)
 * - Can be upgraded to clock algorithm or true LRU later
 *
 * Corresponds to BufferMgr in Rust (NMDB2/src/buffer/buffermgr.rs)
 */
class BufferMgr {
public:
    /**
     * Creates a buffer manager with the specified pool size.
     *
     * @param fm the file manager
     * @param lm the log manager
     * @param numbuffs the number of buffers in the pool
     */
    BufferMgr(std::shared_ptr<file::FileMgr> fm,
              std::shared_ptr<log::LogMgr> lm,
              size_t numbuffs);

    /**
     * Returns the number of available (unpinned) buffers.
     *
     * @return available buffer count
     */
    size_t available() const;

    /**
     * Flushes all buffers modified by the specified transaction.
     *
     * @param txnum the transaction number
     */
    void flush_all(size_t txnum);

    /**
     * Pins a buffer to the specified block.
     *
     * If the block is already in the pool, returns its index.
     * Otherwise, allocates an unpinned buffer and assigns it to the block.
     * If no unpinned buffers are available, waits up to MAX_TIME ms.
     *
     * @param blk the block to pin
     * @return the buffer index
     * @throws BufferAbortException if no buffer available after timeout
     */
    size_t pin(const file::BlockId& blk);

    /**
     * Unpins the buffer at the specified index.
     *
     * @param idx the buffer index
     */
    void unpin(size_t idx);

    /**
     * Returns a reference to the buffer at the specified index.
     *
     * @param idx the buffer index
     * @return reference to the buffer
     */
    Buffer& buffer(size_t idx);

    /**
     * Sets the maximum wait time in milliseconds.
     * (For testing purposes)
     *
     * @param max_time_ms the timeout in milliseconds
     */
    void set_max_time(uint64_t max_time_ms);

private:
    /**
     * Attempts to pin a buffer to the block without waiting.
     *
     * @param blk the block to pin
     * @return the buffer index, or std::nullopt if pool is full
     */
    std::optional<size_t> try_to_pin(const file::BlockId& blk);

    /**
     * Finds a buffer already assigned to the block.
     *
     * @param blk the block to find
     * @return the buffer index, or std::nullopt if not found
     */
    std::optional<size_t> find_existing_buffer(const file::BlockId& blk);

    /**
     * Chooses an unpinned buffer for eviction.
     * Simple strategy: first unpinned buffer found.
     *
     * @return the buffer index, or std::nullopt if all pinned
     */
    std::optional<size_t> choose_unpinned_buffer();

    /**
     * Checks if waiting time has exceeded the maximum.
     *
     * @param start_time the time when waiting started
     * @return true if timeout expired
     */
    bool waiting_too_long(
        const std::chrono::steady_clock::time_point& start_time) const;

private:
    static constexpr uint64_t MAX_TIME = 10000;  // 10 seconds in ms

    std::vector<Buffer> bufferpool_;
    size_t num_available_;
    uint64_t max_time_;
};

} // namespace buffer

#endif // BUFFERMGR_HPP
```

### Implementation Notes

#### Constructor
```cpp
BufferMgr::BufferMgr(std::shared_ptr<file::FileMgr> fm,
                     std::shared_ptr<log::LogMgr> lm,
                     size_t numbuffs)
    : num_available_(numbuffs),
      max_time_(MAX_TIME) {
    bufferpool_.reserve(numbuffs);
    for (size_t i = 0; i < numbuffs; i++) {
        bufferpool_.emplace_back(fm, lm);
    }
}
```

#### Key Methods

**`pin()` - Main entry point**
```cpp
size_t BufferMgr::pin(const file::BlockId& blk) {
    auto start_time = std::chrono::steady_clock::now();

    std::optional<size_t> idx = try_to_pin(blk);

    // Wait loop if pool is full
    while (!idx.has_value() && !waiting_too_long(start_time)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        idx = try_to_pin(blk);
    }

    if (!idx.has_value()) {
        throw BufferAbortException();
    }

    return idx.value();
}
```

**`try_to_pin()` - Core allocation logic**
```cpp
std::optional<size_t> BufferMgr::try_to_pin(const file::BlockId& blk) {
    // First, check if block already in pool
    std::optional<size_t> idx = find_existing_buffer(blk);

    // If not found, allocate a new buffer
    if (!idx.has_value()) {
        idx = choose_unpinned_buffer();
        if (idx.has_value()) {
            bufferpool_[idx.value()].assign_to_block(blk);
        } else {
            return std::nullopt;  // Pool is full
        }
    }

    // Increment pin count
    if (idx.has_value()) {
        if (!bufferpool_[idx.value()].is_pinned()) {
            num_available_--;  // Transitioning from unpinned to pinned
        }
        bufferpool_[idx.value()].pin();
    }

    return idx;
}
```

**`unpin()` - Decrement pin count**
```cpp
void BufferMgr::unpin(size_t idx) {
    Buffer& buff = bufferpool_[idx];
    buff.unpin();

    if (!buff.is_pinned()) {
        num_available_++;
        // In multi-threaded version, would notify waiting threads here
    }
}
```

**`find_existing_buffer()` - Check if block in pool**
```cpp
std::optional<size_t> BufferMgr::find_existing_buffer(const file::BlockId& blk) {
    for (size_t i = 0; i < bufferpool_.size(); i++) {
        const auto& block_opt = bufferpool_[i].block();
        if (block_opt.has_value() && block_opt.value() == blk) {
            return i;
        }
    }
    return std::nullopt;
}
```

**`choose_unpinned_buffer()` - Simple eviction**
```cpp
std::optional<size_t> BufferMgr::choose_unpinned_buffer() {
    for (size_t i = 0; i < bufferpool_.size(); i++) {
        if (!bufferpool_[i].is_pinned()) {
            return i;
        }
    }
    return std::nullopt;  // All buffers are pinned
}
```

**`flush_all()` - Flush transaction's dirty buffers**
```cpp
void BufferMgr::flush_all(size_t txnum) {
    for (auto& buff : bufferpool_) {
        auto tx = buff.modifying_tx();
        if (tx.has_value() && tx.value() == txnum) {
            buff.flush();
        }
    }
}
```

### Test Coverage (15 tests)

**File:** `tests/test_buffermgr.cpp`

```cpp
TEST(BufferMgrTest, InitialState) {
    // All buffers available initially
    BufferMgr bm(fm, lm, 3);
    EXPECT_EQ(bm.available(), 3);
}

TEST(BufferMgrTest, PinReducesAvailable) {
    // Pin reduces available count
}

TEST(BufferMgrTest, UnpinIncreasesAvailable) {
    // Unpin increases available count
}

TEST(BufferMgrTest, PinSameBlockTwice) {
    // Pinning same block twice returns same index
}

TEST(BufferMgrTest, MultiplePinsSameBuffer) {
    // Multiple pins require multiple unpins
}

TEST(BufferMgrTest, PinDifferentBlocks) {
    // Pinning different blocks uses different buffers
}

TEST(BufferMgrTest, ExhaustPool) {
    // Pinning more blocks than pool size waits/aborts
}

TEST(BufferMgrTest, Eviction) {
    // When pool full, unpinned buffer is evicted
}

TEST(BufferMgrTest, FlushAll) {
    // flush_all flushes only specified transaction's buffers
}

TEST(BufferMgrTest, PinAfterUnpin) {
    // Unpin makes buffer available for reuse
}

TEST(BufferMgrTest, BufferContentsPersist) {
    // Modifications to buffer.contents() are visible after unpin/repin
}

TEST(BufferMgrTest, WALComplianceOnEviction) {
    // Eviction of dirty buffer flushes log first
}

TEST(BufferMgrTest, TimeoutThrows) {
    // set_max_time(100), exhaust pool, verify exception
}

TEST(BufferMgrTest, ConcurrentPinUnpin) {
    // (Future) Multi-threaded test
}

TEST(BufferMgrTest, LargePool) {
    // Test with 100 buffers, verify performance
}
```

---

## Integration with Existing Layers

### Dependencies

**Phase 1 (File Layer):**
```cpp
#include "file/filemgr.hpp"
#include "file/page.hpp"
#include "file/blockid.hpp"
```

**Phase 2 (Logging Layer):**
```cpp
#include "log/logmgr.hpp"
```

### Usage Pattern

```cpp
// Initialize
auto fm = std::make_shared<FileMgr>("/tmp/testdb", 400);
auto lm = std::make_shared<LogMgr>(fm, "logfile");
auto bm = std::make_shared<BufferMgr>(fm, lm, 8);  // 8 buffers

// Pin a block
BlockId blk("students.tbl", 0);
size_t idx = bm->pin(blk);

// Modify the buffer
Buffer& buf = bm->buffer(idx);
buf.contents().set_int(0, 123);
buf.set_modified(1, std::nullopt);  // txnum=1, no lsn yet

// Unpin when done
bm->unpin(idx);

// Flush all buffers for transaction 1
bm->flush_all(1);
```

---

## CMakeLists.txt Updates

### Add Buffer Sources

```cmake
# In src/CMakeLists.txt
add_library(mudop_buffer
    buffer/buffer.cpp
    buffer/buffermgr.cpp
)
target_include_directories(mudop_buffer PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(mudop_buffer PUBLIC mudop_file mudop_log)
```

### Link in Main Library

```cmake
target_link_libraries(mudop_utils PRIVATE
    mudop_file
    mudop_log
    mudop_buffer  # <-- Add this
)
```

### Add Tests

```cmake
# In tests/CMakeLists.txt
add_executable(tests
    # ... existing tests ...
    test_buffer.cpp
    test_buffermgr.cpp
)
target_link_libraries(tests PRIVATE gtest_main mudop_utils)
```

---

## Thread Safety Considerations

### Current Design (Single-Threaded)
The initial implementation will be **single-threaded** for simplicity:
- No mutexes in `Buffer` or `BufferMgr`
- Simple sleep loop in `pin()` waiting strategy

### Future Enhancement (Multi-Threaded)
For Phase 6+ (concurrent transactions), add:

1. **Mutex per buffer:** Protect pin count, assignment
2. **Condition variable in BufferMgr:** Notify waiting threads on unpin
3. **Fine-grained locking:** Only lock during buffer allocation, not during page access

```cpp
// Future multi-threaded design
class Buffer {
private:
    std::mutex mutex_;
    std::condition_variable cv_;
    // ...
};

class BufferMgr {
    void unpin(size_t idx) {
        std::lock_guard<std::mutex> lock(bufferpool_[idx].mutex_);
        bufferpool_[idx].unpin();
        if (!bufferpool_[idx].is_pinned()) {
            num_available_++;
            cv_.notify_one();  // Wake up waiting threads
        }
    }
};
```

---

## Testing Strategy

### Unit Tests (10 Buffer + 15 BufferMgr = 25 tests)

**Buffer Tests:**
- State transitions (unpinned → pinned → unpinned)
- Modification tracking
- Flush semantics (WAL compliance)
- Reassignment

**BufferMgr Tests:**
- Pin/unpin correctness
- Pool exhaustion and eviction
- Timeout behavior
- flush_all correctness

### Integration Tests

```cpp
TEST(BufferIntegration, WriteReadCycle) {
    // Write data via buffer, flush, read back in new buffer
}

TEST(BufferIntegration, MultipleTransactions) {
    // Simulate two transactions modifying different blocks
}

TEST(BufferIntegration, RecoverySimulation) {
    // Write dirty buffers, simulate crash (no flush), verify log
}
```

### Performance Tests

```cpp
TEST(BufferPerformance, HitRate) {
    // Pin same blocks repeatedly, measure hit rate
}

TEST(BufferPerformance, EvictionOverhead) {
    // Time eviction when pool is full
}
```

---

## Implementation Checklist

### Phase 3 Tasks

- [ ] **1. Design Review** (0.5 day)
  - Review Rust reference implementation
  - Finalize C++ API design
  - Document differences from Rust

- [ ] **2. Implement Buffer** (1 day)
  - [ ] Create `include/buffer/buffer.hpp`
  - [ ] Create `src/buffer/buffer.cpp`
  - [ ] Implement constructor, getters
  - [ ] Implement `assign_to_block()`, `flush()`
  - [ ] Implement `pin()`, `unpin()`
  - [ ] Implement `set_modified()`

- [ ] **3. Test Buffer** (0.5 day)
  - [ ] Create `tests/test_buffer.cpp`
  - [ ] Write 10 unit tests
  - [ ] Verify all tests pass

- [ ] **4. Implement BufferMgr** (1 day)
  - [ ] Create `include/buffer/buffermgr.hpp`
  - [ ] Create `src/buffer/buffermgr.cpp`
  - [ ] Implement constructor, `available()`
  - [ ] Implement `try_to_pin()`, `pin()`, `unpin()`
  - [ ] Implement `find_existing_buffer()`, `choose_unpinned_buffer()`
  - [ ] Implement `flush_all()`

- [ ] **5. Test BufferMgr** (1 day)
  - [ ] Create `tests/test_buffermgr.cpp`
  - [ ] Write 15 unit tests
  - [ ] Write 3 integration tests
  - [ ] Verify all tests pass

- [ ] **6. Build and Verify** (0.5 day)
  - [ ] Update CMakeLists.txt
  - [ ] Build via Docker
  - [ ] Run full test suite (expect 124 + 25 = 149 tests)
  - [ ] Verify 100% pass rate

- [ ] **7. Documentation** (0.5 day)
  - [ ] Update architecture overview
  - [ ] Write Phase 3 completion report
  - [ ] Document API with examples

**Total:** 3.5-4.5 days

---

## Success Criteria

### Phase 3 Complete When:
- ✅ All 25 buffer tests passing (100%)
- ✅ Integration with Phases 1 & 2 verified
- ✅ Docker build successful
- ✅ Code reviewed and documented
- ✅ WAL compliance verified in tests
- ✅ No memory leaks (valgrind clean)

---

## Files to Create Summary

### Headers (2 files)
1. `include/buffer/buffer.hpp` (~100 lines)
2. `include/buffer/buffermgr.hpp` (~120 lines)

### Implementation (2 files)
3. `src/buffer/buffer.cpp` (~150 lines)
4. `src/buffer/buffermgr.cpp` (~200 lines)

### Tests (2 files)
5. `tests/test_buffer.cpp` (~400 lines)
6. `tests/test_buffermgr.cpp` (~600 lines)

### Build Configuration (1 file)
7. `src/CMakeLists.txt` (modified, add ~10 lines)

**Total New Code:** ~1570 lines

---

## Risk Mitigation

### Risk 1: Concurrency Bugs
**Mitigation:** Start with single-threaded design, add locks in Phase 6

### Risk 2: Eviction Policy Inefficiency
**Mitigation:** Use simple first-unpinned strategy, upgrade to clock/LRU if needed

### Risk 3: Pin/Unpin Mismatch
**Mitigation:** Comprehensive tests, consider RAII wrapper (future)

### Risk 4: WAL Violation
**Mitigation:** Explicit tests for flush ordering

---

## Next Phase Preview

After Phase 3 completes, Phase 4 (Record Management) will build on BufferMgr:

```cpp
// Phase 4 usage example
class TableScan {
    std::shared_ptr<BufferMgr> bm_;
    BlockId current_blk_;
    size_t buffer_idx_;

    void move_to_block(BlockId blk) {
        if (buffer_idx_) bm_->unpin(*buffer_idx_);
        buffer_idx_ = bm_->pin(blk);
        // Access buffer via bm_->buffer(buffer_idx_)
    }
};
```

---

**Document Version:** 1.0
**Last Updated:** October 12, 2025
**Status:** Ready for Implementation
