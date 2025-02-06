#include "buffer/buffer.hpp"

namespace buffer {

Buffer::Buffer(std::shared_ptr<file::FileMgr> fm,
               std::shared_ptr<log::LogMgr> lm)
    : fm_(fm),
      lm_(lm),
      contents_(fm->block_size()),
      blk_(std::nullopt),
      pins_(0),
      txnum_(std::nullopt),
      lsn_(std::nullopt) {
}

file::Page& Buffer::contents() {
    return contents_;
}

const std::optional<file::BlockId>& Buffer::block() const {
    return blk_;
}

void Buffer::set_modified(size_t txnum, std::optional<size_t> lsn) {
    txnum_ = txnum;
    if (lsn.has_value()) {
        lsn_ = lsn;
    }
}

bool Buffer::is_pinned() const {
    return pins_ > 0;
}

std::optional<size_t> Buffer::modifying_tx() const {
    return txnum_;
}

void Buffer::assign_to_block(const file::BlockId& blk) {
    // Flush old block if dirty
    flush();

    // Assign to new block
    blk_ = blk;
    fm_->read(blk, contents_);
    pins_ = 0;
}

void Buffer::flush() {
    // Only flush if buffer has been modified
    if (txnum_.has_value()) {
        // WAL: Flush log first
        if (lsn_.has_value()) {
            lm_->flush(lsn_.value());
        }

        // Then flush data page
        if (blk_.has_value()) {
            fm_->write(blk_.value(), contents_);
        }

        // Mark as clean
        txnum_ = std::nullopt;
    }
}

void Buffer::pin() {
    pins_++;
}

void Buffer::unpin() {
    pins_--;
}

} // namespace buffer
