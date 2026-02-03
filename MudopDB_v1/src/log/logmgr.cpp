#include "log/logmgr.hpp"
#include <stdexcept>

namespace log {

// Static helper function
file::BlockId LogMgr::append_new_block_helper(
    file::FileMgr* fm,
    file::Page& logpage,
    const std::string& logfile
) {
    file::BlockId blk = fm->append(logfile);
    // Set boundary to end of page (records will grow backward from here)
    logpage.set_int(0, static_cast<int32_t>(fm->block_size()));
    fm->write(blk, logpage);
    return blk;
}

LogMgr::LogMgr(std::shared_ptr<file::FileMgr> fm, const std::string& logfile)
    : fm_(fm),
      logfile_(logfile),
      logpage_(fm->block_size()),
      currentblk_("", 0),
      latest_lsn_(0),
      last_saved_lsn_(0) {

    size_t logsize = fm_->length(logfile_);

    if (logsize == 0) {
        // New log file - create first block
        currentblk_ = append_new_block_helper(fm_.get(), logpage_, logfile_);
    } else {
        // Existing log file - load the last block
        currentblk_ = file::BlockId(logfile_, static_cast<int32_t>(logsize) - 1);
        fm_->read(currentblk_, logpage_);
    }
}

size_t LogMgr::append(const std::vector<uint8_t>& logrec) {
    // Get current boundary (first free position in page)
    int32_t boundary = logpage_.get_int(0);

    // Calculate space needed: 4 bytes for length + record data
    int32_t recsize = static_cast<int32_t>(logrec.size());
    int32_t bytesneeded = recsize + 4;

    // Check if record fits in current page
    // Need to leave at least 4 bytes for the boundary itself
    if (boundary - bytesneeded < 4) {
        // Page is full - flush and allocate new block
        flush_impl();
        currentblk_ = append_new_block();
        boundary = logpage_.get_int(0);
    }

    // Calculate position for new record (grows backward)
    int32_t recpos = boundary - bytesneeded;

    // Write the record
    logpage_.set_bytes(static_cast<size_t>(recpos), logrec.data(), logrec.size());

    // Update boundary to point to start of this record
    logpage_.set_int(0, recpos);

    // Increment and return LSN
    latest_lsn_++;
    return latest_lsn_;
}

void LogMgr::flush(size_t lsn) {
    // Only flush if the requested LSN hasn't been saved yet
    if (lsn >= last_saved_lsn_) {
        flush_impl();
    }
}

std::unique_ptr<LogIterator> LogMgr::iterator() {
    // Flush to ensure all records are on disk
    flush_impl();
    // Create iterator starting at current block
    return std::make_unique<LogIterator>(fm_, currentblk_);
}

file::BlockId LogMgr::append_new_block() {
    return append_new_block_helper(fm_.get(), logpage_, logfile_);
}

void LogMgr::flush_impl() {
    fm_->write(currentblk_, logpage_);
    last_saved_lsn_ = latest_lsn_;
}

} // namespace log
