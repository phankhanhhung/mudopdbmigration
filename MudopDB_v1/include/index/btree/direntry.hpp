#ifndef DIRENTRY_HPP
#define DIRENTRY_HPP

#include "query/constant.hpp"

namespace index {

/**
 * Directory entry in B-Tree (dataval + block number).
 *
 * Corresponds to DirEntry in Rust (NMDB2/src/index/btree/direntry.rs)
 */
class DirEntry {
public:
    DirEntry(const Constant& dataval, int32_t blocknum)
        : dataval_(dataval), blocknum_(blocknum) {}

    Constant data_val() const { return dataval_; }
    int32_t block_number() const { return blocknum_; }

private:
    Constant dataval_;
    int32_t blocknum_;
};

} // namespace index

#endif // DIRENTRY_HPP
