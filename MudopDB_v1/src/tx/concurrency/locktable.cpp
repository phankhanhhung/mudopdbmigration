#include "tx/concurrency/locktable.hpp"
#include "buffer/buffermgr.hpp"

namespace tx {

LockTable::LockTable()
    : max_time_(MAX_TIME) {}

void LockTable::s_lock(const file::BlockId& blk) {
    std::unique_lock<std::mutex> lock(mutex_);
    auto deadline = std::chrono::steady_clock::now()
                  + std::chrono::milliseconds(max_time_);

    // Wait until no exclusive lock exists on this block
    bool ok = cv_.wait_until(lock, deadline, [&] {
        return !has_x_lock(blk);
    });

    if (!ok) {
        throw buffer::BufferAbortException();
    }

    int32_t val = get_lock_val(blk);
    locks_[blk] = val + 1;
}

void LockTable::x_lock(const file::BlockId& blk) {
    std::unique_lock<std::mutex> lock(mutex_);
    auto deadline = std::chrono::steady_clock::now()
                  + std::chrono::milliseconds(max_time_);

    // Wait until no other S locks exist (val must be 1, our own S lock)
    bool ok = cv_.wait_until(lock, deadline, [&] {
        return !has_other_s_locks(blk);
    });

    if (!ok) {
        throw buffer::BufferAbortException();
    }

    // Set exclusive lock (negative value)
    locks_[blk] = -1;
}

void LockTable::unlock(const file::BlockId& blk) {
    std::lock_guard<std::mutex> lock(mutex_);
    int32_t val = get_lock_val(blk);
    if (val > 1) {
        locks_[blk] = val - 1;
    } else {
        locks_.erase(blk);
    }
    // Notify all waiters that a lock was released
    cv_.notify_all();
}

bool LockTable::has_x_lock(const file::BlockId& blk) const {
    return get_lock_val(blk) < 0;
}

bool LockTable::has_other_s_locks(const file::BlockId& blk) const {
    return get_lock_val(blk) > 1;
}

int32_t LockTable::get_lock_val(const file::BlockId& blk) const {
    auto it = locks_.find(blk);
    if (it != locks_.end()) {
        return it->second;
    }
    return 0;
}

} // namespace tx
