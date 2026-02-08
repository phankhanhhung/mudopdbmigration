#include "tx/concurrency/concurrencymgr.hpp"
#include "tx/concurrency/locktable.hpp"

namespace tx {

// Global lock table singleton (corresponds to Rust's static LOCKTBL)
static LockTable& get_lock_table() {
    static LockTable instance;
    return instance;
}

ConcurrencyMgr::ConcurrencyMgr() {}

void ConcurrencyMgr::s_lock(const file::BlockId& blk) {
    if (locks_.find(blk) == locks_.end()) {
        get_lock_table().s_lock(blk);
        locks_[blk] = LockType::Shared;
    }
}

void ConcurrencyMgr::x_lock(const file::BlockId& blk) {
    if (!has_x_lock(blk)) {
        s_lock(blk);
        get_lock_table().x_lock(blk);
        locks_[blk] = LockType::Exclusive;
    }
}

void ConcurrencyMgr::release() {
    for (const auto& pair : locks_) {
        get_lock_table().unlock(pair.first);
    }
    locks_.clear();
}

bool ConcurrencyMgr::has_x_lock(const file::BlockId& blk) const {
    auto it = locks_.find(blk);
    if (it != locks_.end()) {
        return it->second == LockType::Exclusive;
    }
    return false;
}

} // namespace tx
