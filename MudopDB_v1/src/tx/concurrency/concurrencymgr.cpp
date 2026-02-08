#include "tx/concurrency/concurrencymgr.hpp"
#include "tx/concurrency/locktable.hpp"
#include <mutex>

namespace tx {

// Global lock table singleton (corresponds to Rust's static LOCKTBL)
static LockTable& get_lock_table() {
    static LockTable instance;
    return instance;
}
static std::mutex& get_lock_table_mutex() {
    static std::mutex mtx;
    return mtx;
}

ConcurrencyMgr::ConcurrencyMgr() {}

void ConcurrencyMgr::s_lock(const file::BlockId& blk) {
    // Check and acquire atomically under the same lock to prevent TOCTOU race
    std::lock_guard<std::mutex> lock(get_lock_table_mutex());
    if (locks_.find(blk) == locks_.end()) {
        get_lock_table().s_lock(blk);
        locks_[blk] = "S";
    }
}

void ConcurrencyMgr::x_lock(const file::BlockId& blk) {
    if (!has_x_lock(blk)) {
        // First acquire S lock if we don't have any lock
        s_lock(blk);
        // Then upgrade to X lock on the global table
        std::lock_guard<std::mutex> lock(get_lock_table_mutex());
        get_lock_table().x_lock(blk);
        locks_[blk] = "X";
    }
}

void ConcurrencyMgr::release() {
    {
        std::lock_guard<std::mutex> lock(get_lock_table_mutex());
        for (const auto& pair : locks_) {
            get_lock_table().unlock(pair.first);
        }
    }
    locks_.clear();
}

bool ConcurrencyMgr::has_x_lock(const file::BlockId& blk) const {
    auto it = locks_.find(blk);
    if (it != locks_.end()) {
        return it->second == "X";
    }
    return false;
}

} // namespace tx
