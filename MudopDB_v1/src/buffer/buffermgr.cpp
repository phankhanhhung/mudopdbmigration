#include "buffer/buffermgr.hpp"

namespace buffer {

BufferMgr::BufferMgr(std::shared_ptr<file::FileMgr> fm,
                     std::shared_ptr<log::LogMgr> lm,
                     size_t numbuffs)
    : fm_(fm),
      num_available_(numbuffs),
      max_time_(MAX_TIME) {
    bufferpool_.reserve(numbuffs);
    for (size_t i = 0; i < numbuffs; i++) {
        bufferpool_.emplace_back(fm, lm);
    }
}

size_t BufferMgr::available() const {
    return num_available_;
}

void BufferMgr::flush_all(size_t txnum) {
    for (auto& buff : bufferpool_) {
        auto tx = buff.modifying_tx();
        if (tx.has_value() && tx.value() == txnum) {
            buff.flush();
        }
    }
}

size_t BufferMgr::pin(const file::BlockId& blk) {
    auto start_time = std::chrono::steady_clock::now();

    std::optional<size_t> idx = try_to_pin(blk);

    // Wait loop if pool is full
    while (!idx.has_value() && !waiting_too_long(start_time)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        idx = try_to_pin(blk);
    }

    if (!idx.has_value()) {
        throw BufferAbortException();
    }

    return idx.value();
}

void BufferMgr::unpin(size_t idx) {
    Buffer& buff = bufferpool_[idx];
    buff.unpin();

    if (!buff.is_pinned()) {
        num_available_++;
        // In multi-threaded version, would notify waiting threads here
    }
}

Buffer& BufferMgr::buffer(size_t idx) {
    return bufferpool_[idx];
}

void BufferMgr::set_max_time(uint64_t max_time_ms) {
    max_time_ = max_time_ms;
}

std::shared_ptr<file::FileMgr> BufferMgr::file_mgr() const {
    return fm_;
}

std::optional<size_t> BufferMgr::try_to_pin(const file::BlockId& blk) {
    // First, check if block already in pool
    std::optional<size_t> idx = find_existing_buffer(blk);

    // If not found, allocate a new buffer
    if (!idx.has_value()) {
        idx = choose_unpinned_buffer();
        if (idx.has_value()) {
            bufferpool_[idx.value()].assign_to_block(blk);
        } else {
            return std::nullopt;  // Pool is full
        }
    }

    // Increment pin count
    if (idx.has_value()) {
        if (!bufferpool_[idx.value()].is_pinned()) {
            num_available_--;  // Transitioning from unpinned to pinned
        }
        bufferpool_[idx.value()].pin();
    }

    return idx;
}

std::optional<size_t> BufferMgr::find_existing_buffer(const file::BlockId& blk) {
    for (size_t i = 0; i < bufferpool_.size(); i++) {
        const auto& block_opt = bufferpool_[i].block();
        if (block_opt.has_value() && block_opt.value() == blk) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<size_t> BufferMgr::choose_unpinned_buffer() {
    for (size_t i = 0; i < bufferpool_.size(); i++) {
        if (!bufferpool_[i].is_pinned()) {
            return i;
        }
    }
    return std::nullopt;  // All buffers are pinned
}

bool BufferMgr::waiting_too_long(
    const std::chrono::steady_clock::time_point& start_time) const {
    auto elapsed = std::chrono::steady_clock::now() - start_time;
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    return static_cast<uint64_t>(elapsed_ms) > max_time_;
}

} // namespace buffer
