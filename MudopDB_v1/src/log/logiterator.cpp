#include "log/logiterator.hpp"
#include <stdexcept>

namespace log {

LogIterator::LogIterator(std::shared_ptr<file::FileMgr> fm, const file::BlockId& blk)
    : fm_(fm), blk_(blk), page_(fm->block_size()), currentpos_(0), boundary_(0) {
    move_to_block(blk);
}

void LogIterator::move_to_block(const file::BlockId& blk) {
    fm_->read(blk, page_);
    boundary_ = page_.get_int(0);
    currentpos_ = boundary_;
}

bool LogIterator::has_next() const {
    // Check if we have more records in current page
    if (currentpos_ < static_cast<int32_t>(fm_->block_size())) {
        return true;
    }

    // Check if there are previous blocks to read
    return blk_.number() > 0;
}

std::vector<uint8_t> LogIterator::next() {
    // If current page is exhausted and we're at block 0, no more records
    if (currentpos_ >= static_cast<int32_t>(fm_->block_size()) && blk_.number() <= 0) {
        throw std::runtime_error("No more log records");
    }

    // If current page is exhausted, move to previous block
    if (currentpos_ >= static_cast<int32_t>(fm_->block_size())) {
        blk_ = file::BlockId(blk_.file_name(), blk_.number() - 1);
        move_to_block(blk_);
    }

    // Read the record at currentpos
    const uint8_t* rec_data = page_.get_bytes(static_cast<size_t>(currentpos_));
    size_t rec_len = page_.get_bytes_length(static_cast<size_t>(currentpos_));

    // Create vector from the record data
    std::vector<uint8_t> record(rec_data, rec_data + rec_len);

    // Advance position: 4 bytes for length + actual record length
    currentpos_ += 4 + static_cast<int32_t>(rec_len);

    return record;
}

} // namespace log
