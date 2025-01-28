#ifndef LOGITERATOR_HPP
#define LOGITERATOR_HPP

#include "file/blockid.hpp"
#include "file/page.hpp"
#include "file/filemgr.hpp"
#include <memory>
#include <vector>
#include <cstdint>

namespace log {

class LogIterator {
public:
    LogIterator(std::shared_ptr<file::FileMgr> fm, const file::BlockId& blk);

    bool has_next() const;

    std::vector<uint8_t> next();

private:
    std::shared_ptr<file::FileMgr> fm_;
    file::BlockId blk_;
    file::Page page_;
    int32_t currentpos_;
    int32_t boundary_;

    void move_to_block(const file::BlockId& blk);
};

} // namespace log

#endif // LOGITERATOR_HPP
