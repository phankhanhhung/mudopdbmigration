#ifndef MEMFILEMGR_HPP
#define MEMFILEMGR_HPP

#include "file/filemgr.hpp"
#include <unordered_map>
#include <vector>
#include <mutex>

namespace file {

/**
 * MemFileMgr is an in-memory implementation of FileMgr.
 * All data is stored in memory (no disk I/O).
 * Data is lost when the process exits.
 *
 * Thread-safe: all operations are protected by a mutex.
 */
class MemFileMgr : public FileMgr {
public:
    explicit MemFileMgr(size_t blocksize);

    void read(const BlockId& blk, Page& page) override;
    void write(const BlockId& blk, Page& page) override;
    BlockId append(const std::string& filename) override;
    size_t length(const std::string& filename) override;
    bool is_new() const override;
    size_t block_size() const override;

private:
    size_t blocksize_;
    // filename -> vector of blocks, each block is a vector<uint8_t>
    std::unordered_map<std::string, std::vector<std::vector<uint8_t>>> files_;
    mutable std::mutex mutex_;
};

} // namespace file

#endif // MEMFILEMGR_HPP
