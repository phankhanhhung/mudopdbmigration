#ifndef CONCURRENCYMGR_HPP
#define CONCURRENCYMGR_HPP

#include "file/blockid.hpp"
#include <unordered_map>
#include <string>

namespace tx {

/**
 * ConcurrencyMgr manages per-transaction lock state.
 * Each transaction has its own ConcurrencyMgr that tracks S/X locks.
 * Uses a global LockTable singleton for coordination.
 *
 * Corresponds to ConcurrencyMgr in Rust (NMDB2/src/tx/concurrency/concurrencymgr.rs)
 */
class ConcurrencyMgr {
public:
    ConcurrencyMgr();

    void s_lock(const file::BlockId& blk);
    void x_lock(const file::BlockId& blk);
    void release();

private:
    bool has_x_lock(const file::BlockId& blk) const;

    std::unordered_map<file::BlockId, std::string> locks_;
};

} // namespace tx

#endif // CONCURRENCYMGR_HPP
