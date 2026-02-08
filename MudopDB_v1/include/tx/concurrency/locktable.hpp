#ifndef LOCKTABLE_HPP
#define LOCKTABLE_HPP

#include "file/blockid.hpp"
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <cstdint>

namespace tx {

/**
 * LockTable manages the global lock state for blocks.
 * Positive lock values indicate shared lock count.
 * Negative lock values indicate exclusive lock (-1).
 *
 * Thread Safety: This class is thread-safe. All methods are protected
 * by an internal mutex with a condition_variable for fair waiting.
 * Waiters are notified when locks are released, eliminating busy-polling.
 *
 * Corresponds to LockTable in Rust (NMDB2/src/tx/concurrency/locktable.rs)
 */
class LockTable {
public:
    LockTable();

    /**
     * Acquires a shared lock on the block.
     * Waits (with cv) if an exclusive lock exists. Throws on timeout.
     */
    void s_lock(const file::BlockId& blk);

    /**
     * Acquires an exclusive lock on the block.
     * Caller must already hold an S lock (lock val >= 1).
     * Waits (with cv) if other S locks exist (val > 1). Throws on timeout.
     */
    void x_lock(const file::BlockId& blk);

    /**
     * Releases the lock on the block and notifies waiting threads.
     */
    void unlock(const file::BlockId& blk);

private:
    static constexpr uint64_t MAX_TIME = 10000; // 10 seconds

    bool has_x_lock(const file::BlockId& blk) const;
    bool has_other_s_locks(const file::BlockId& blk) const;
    int32_t get_lock_val(const file::BlockId& blk) const;

    std::unordered_map<file::BlockId, int32_t> locks_;
    uint64_t max_time_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace tx

#endif // LOCKTABLE_HPP
