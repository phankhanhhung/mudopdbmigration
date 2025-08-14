#include "file/blockid.hpp"
#include <sstream>

namespace file {

BlockId::BlockId(const std::string& filename, int32_t blknum)
    : filename_(filename), blknum_(blknum) {}

const std::string& BlockId::file_name() const {
    return filename_;
}

int32_t BlockId::number() const {
    return blknum_;
}

std::string BlockId::to_string() const {
    std::ostringstream oss;
    oss << "[file " << filename_ << ", block " << blknum_ << "]";
    return oss.str();
}

bool BlockId::operator==(const BlockId& other) const {
    return filename_ == other.filename_ && blknum_ == other.blknum_;
}

bool BlockId::operator!=(const BlockId& other) const {
    return !(*this == other);
}

bool BlockId::operator<(const BlockId& other) const {
    if (filename_ != other.filename_) {
        return filename_ < other.filename_;
    }
    return blknum_ < other.blknum_;
}

} // namespace file

// Hash function implementation
namespace std {
    size_t hash<file::BlockId>::operator()(const file::BlockId& blk) const noexcept {
        // Combine hash of filename and block number
        size_t h1 = hash<string>()(blk.file_name());
        size_t h2 = hash<int32_t>()(blk.number());
        // Standard hash combination formula
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
}
