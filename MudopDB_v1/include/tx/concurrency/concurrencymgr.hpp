#ifndef CONCURRENCYMGR_HPP
#define CONCURRENCYMGR_HPP

#include "file/blockid.hpp"
#include <unordered_map>
#include <string>

namespace tx {

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
