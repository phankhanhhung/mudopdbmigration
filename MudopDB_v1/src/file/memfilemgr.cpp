#include "file/memfilemgr.hpp"
#include <algorithm>
#include <cstring>

namespace file {

MemFileMgr::MemFileMgr(size_t blocksize)
    : FileMgr(blocksize, /*is_new=*/true), blocksize_(blocksize) {}

void MemFileMgr::read(const BlockId& blk, Page& page) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = files_.find(blk.file_name());
    if (it == files_.end()) {
        // File doesn't exist, page stays zeroed
        return;
    }

    size_t blknum = static_cast<size_t>(blk.number());
    if (blknum >= it->second.size()) {
        // Block beyond end of file, page stays zeroed
        return;
    }

    const auto& data = it->second[blknum];
    std::memcpy(page.contents().data(), data.data(),
                std::min(data.size(), page.contents().size()));
}

void MemFileMgr::write(const BlockId& blk, Page& page) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto& blocks = files_[blk.file_name()];
    size_t blknum = static_cast<size_t>(blk.number());

    // Extend file if needed
    while (blocks.size() <= blknum) {
        blocks.emplace_back(blocksize_, 0);
    }

    std::memcpy(blocks[blknum].data(), page.contents().data(),
                std::min(blocks[blknum].size(), page.contents().size()));
}

BlockId MemFileMgr::append(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto& blocks = files_[filename];
    int32_t new_blknum = static_cast<int32_t>(blocks.size());
    blocks.emplace_back(blocksize_, 0);

    return BlockId(filename, new_blknum);
}

size_t MemFileMgr::length(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = files_.find(filename);
    if (it == files_.end()) {
        return 0;
    }
    return it->second.size();
}

bool MemFileMgr::is_new() const {
    return true;  // In-memory DB is always "new"
}

size_t MemFileMgr::block_size() const {
    return blocksize_;
}

} // namespace file
