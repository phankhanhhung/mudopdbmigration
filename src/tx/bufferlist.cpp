#include "tx/bufferlist.hpp"
#include <algorithm>
#include <stdexcept>

namespace tx {

BufferList::BufferList(std::shared_ptr<buffer::BufferMgr> bm)
    : bm_(bm) {}

std::optional<size_t> BufferList::get_index(const file::BlockId& blk) const {
    auto it = buffers_.find(blk);
    if (it != buffers_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void BufferList::pin(const file::BlockId& blk) {
    size_t idx = bm_->pin(blk);
    buffers_[blk] = idx;
    pins_.push_back(blk);
}

void BufferList::unpin(const file::BlockId& blk) {
    auto it = buffers_.find(blk);
    if (it != buffers_.end()) {
        bm_->unpin(it->second);
        auto pos = std::find(pins_.begin(), pins_.end(), blk);
        if (pos != pins_.end()) {
            pins_.erase(pos);
        }
        if (std::find(pins_.begin(), pins_.end(), blk) == pins_.end()) {
            buffers_.erase(blk);
        }
        return;
    }
    throw std::runtime_error("BufferList::unpin: block not found");
}

void BufferList::unpin_all() {
    for (const auto& blk : pins_) {
        auto it = buffers_.find(blk);
        if (it != buffers_.end()) {
            bm_->unpin(it->second);
        }
    }
    buffers_.clear();
    pins_.clear();
}

} // namespace tx
