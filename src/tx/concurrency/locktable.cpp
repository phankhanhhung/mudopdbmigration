/**
 * LockTable implementation.
 * Maintains a table of shared (S) and exclusive (X) locks on blocks.
 * Uses wait-based conflict resolution with timeout detection.
 */

#include "tx/concurrency/locktable.hpp"
#include "buffer/buffermgr.hpp"
#include <thread>

namespace tx {

LockTable::LockTable()
    : max_time_(MAX_TIME) {}

void LockTable::s_lock(const file::BlockId& blk) {
    auto timestamp = std::chrono::steady_clock::now();
    while (has_x_lock(blk) && !waiting_too_long(timestamp)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(max_time_));
    }
    if (has_x_lock(blk)) {
        throw buffer::BufferAbortException();
    }
    int32_t val = get_lock_val(blk);
    locks_[blk] = val + 1;
}

void LockTable::unlock(const file::BlockId& blk) {
    int32_t val = get_lock_val(blk);
    if (val > 1) {
        locks_[blk] = val - 1;
    } else {
        locks_.erase(blk);
    }
}

bool LockTable::has_x_lock(const file::BlockId& blk) const {
    return get_lock_val(blk) < 0;
}

int32_t LockTable::get_lock_val(const file::BlockId& blk) const {
    auto it = locks_.find(blk);
    if (it != locks_.end()) {
        return it->second;
    }
    return 0;
}

bool LockTable::waiting_too_long(
    const std::chrono::steady_clock::time_point& start_time) const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - start_time).count();
    return static_cast<uint64_t>(elapsed) > max_time_;
}

} // namespace tx
