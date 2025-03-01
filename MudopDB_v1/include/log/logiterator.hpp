#ifndef LOGITERATOR_HPP
#define LOGITERATOR_HPP

#include "file/blockid.hpp"
#include "file/page.hpp"
#include "file/filemgr.hpp"
#include <memory>
#include <vector>
#include <cstdint>

namespace log {

/**
 * LogIterator provides backward iteration through log records.
 *
 * The iterator starts at the most recent log record and moves backward
 * through the log file. Within each page, it moves forward (from boundary
 * to end), but pages are traversed in reverse order (newest to oldest).
 *
 * Corresponds to LogIterator in Rust (NMDB2/src/log/logiterator.rs)
 */
class LogIterator {
public:
    /**
     * Creates a new log iterator starting at the specified block.
     * @param fm the file manager
     * @param blk the starting block (usually the last block in the log)
     */
    LogIterator(std::shared_ptr<file::FileMgr> fm, const file::BlockId& blk);

    /**
     * Returns true if there are more log records to read.
     */
    bool has_next() const;

    /**
     * Returns the next log record and advances the iterator.
     * @return the log record as a byte vector
     * @throws std::runtime_error if no more records exist
     */
    std::vector<uint8_t> next();

private:
    std::shared_ptr<file::FileMgr> fm_;
    file::BlockId blk_;
    file::Page page_;
    int32_t currentpos_;
    int32_t boundary_;

    /**
     * Moves the iterator to the specified block and reads its contents.
     */
    void move_to_block(const file::BlockId& blk);
};

} // namespace log

#endif // LOGITERATOR_HPP
