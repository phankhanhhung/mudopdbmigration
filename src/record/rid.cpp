#include "record/rid.hpp"
#include <sstream>

namespace record {

RID::RID(int32_t blknum, size_t slot)
    : blknum_(blknum), slot_(slot) {}

int32_t RID::block_number() const {
    return blknum_;
}

size_t RID::slot() const {
    return slot_;
}

std::string RID::to_string() const {
    std::ostringstream oss;
    oss << "[" << blknum_ << ", " << slot_ << "]";
    return oss.str();
}

bool RID::operator==(const RID& other) const {
    return blknum_ == other.blknum_ && slot_ == other.slot_;
}

bool RID::operator!=(const RID& other) const {
    return !(*this == other);
}

} // namespace record
