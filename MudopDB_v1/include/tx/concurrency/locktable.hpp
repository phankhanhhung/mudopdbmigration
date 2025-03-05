#ifndef LOCKTABLE_HPP
#define LOCKTABLE_HPP

#include "file/blockid.hpp"
#include <unordered_map>
#include <chrono>
#include <cstdint>

namespace tx {

class LockTable {
public:
    LockTable();

    void s_lock(const file::BlockId& blk);
    void unlock(const file::BlockId& blk);

private:
    static constexpr uint64_t MAX_TIME = 10000; // 10 seconds

    bool has_x_lock(const file::BlockId& blk) const;
    int32_t get_lock_val(const file::BlockId& blk) const;
    bool waiting_too_long(
        const std::chrono::steady_clock::time_point& start_time) const;

    std::unordered_map<file::BlockId, int32_t> locks_;
    uint64_t max_time_;
};

} // namespace tx

#endif // LOCKTABLE_HPP
