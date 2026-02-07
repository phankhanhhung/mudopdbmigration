#include "multibuffer/chunkscan.hpp"
#include "file/blockid.hpp"
#include "tx/transaction.hpp"
#include "record/schema.hpp"
#include <stdexcept>

namespace multibuffer {

ChunkScan::ChunkScan(std::shared_ptr<tx::Transaction> tx,
                     const std::string& filename,
                     const record::Layout& layout,
                     size_t startbnum,
                     size_t endbnum)
    : tx_(tx), filename_(filename), layout_(layout),
      startbnum_(startbnum), endbnum_(endbnum),
      currentbnum_(0), rpidx_(0) {
    for (size_t i = startbnum; i <= endbnum; i++) {
        file::BlockId blk(filename, static_cast<int32_t>(i));
        buffs_.emplace_back(tx, blk, layout);
    }
    move_to_block(startbnum);
}

void ChunkScan::move_to_block(size_t blknum) {
    currentbnum_ = blknum;
    rpidx_ = currentbnum_ - startbnum_;
    currentslot_.reset();
}

void ChunkScan::before_first() {
    move_to_block(startbnum_);
}

bool ChunkScan::next() {
    if (rpidx_ < buffs_.size()) {
        currentslot_ = buffs_[rpidx_].next_after(currentslot_);
    }
    while (!currentslot_.has_value()) {
        if (currentbnum_ == endbnum_) {
            return false;
        }
        size_t blknum = buffs_[rpidx_].block().number() + 1;
        move_to_block(blknum);
        if (rpidx_ < buffs_.size()) {
            currentslot_ = buffs_[rpidx_].next_after(currentslot_);
        }
    }
    return true;
}

int32_t ChunkScan::get_int(const std::string& fldname) {
    if (rpidx_ < buffs_.size() && currentslot_.has_value()) {
        return buffs_[rpidx_].get_int(currentslot_.value(), fldname);
    }
    throw std::runtime_error("ChunkScan: no current record");
}

std::string ChunkScan::get_string(const std::string& fldname) {
    if (rpidx_ < buffs_.size() && currentslot_.has_value()) {
        return buffs_[rpidx_].get_string(currentslot_.value(), fldname);
    }
    throw std::runtime_error("ChunkScan: no current record");
}

Constant ChunkScan::get_val(const std::string& fldname) {
    if (layout_.schema()->type(fldname) == record::Type::INTEGER) {
        return Constant::with_int(get_int(fldname));
    }
    return Constant::with_string(get_string(fldname));
}

bool ChunkScan::has_field(const std::string& fldname) const {
    return layout_.schema()->has_field(fldname);
}

void ChunkScan::close() {
    for (size_t i = 0; i < buffs_.size(); i++) {
        file::BlockId blk(filename_, static_cast<int32_t>(startbnum_ + i));
        tx_->unpin(blk);
    }
}

} // namespace multibuffer
