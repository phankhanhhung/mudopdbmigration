#include "multibuffer/multibufferproductscan.hpp"
#include "multibuffer/bufferneeds.hpp"
#include "multibuffer/chunkscan.hpp"
#include "tx/transaction.hpp"
#include <stdexcept>

namespace multibuffer {

MultibufferProductScan::MultibufferProductScan(std::shared_ptr<tx::Transaction> tx,
                                               std::unique_ptr<Scan> lhsscan,
                                               const std::string& tblname,
                                               const record::Layout& layout)
    : tx_(tx), lhsscan_(std::move(lhsscan)),
      filename_(tblname + ".tbl"), layout_(layout),
      nextblknum_(0), filesize_(0) {
    filesize_ = tx_->size(filename_);
    size_t available = tx_->available_buffs();
    chunksize_ = BufferNeeds::best_factor(available, filesize_);
    before_first();
}

void MultibufferProductScan::before_first() {
    nextblknum_ = 0;
    use_next_chunk();
}

bool MultibufferProductScan::next() {
    // Nested loop: for each RHS chunk, scan all LHS rows paired with all chunk rows
    while (rhsscan_) {
        // Try advancing RHS within current LHS row
        if (rhsscan_->next()) {
            return true;
        }
        // RHS exhausted, advance LHS
        if (!lhsscan_->next()) {
            // LHS exhausted for this chunk, move to next chunk
            if (!use_next_chunk()) {
                return false;
            }
            // use_next_chunk resets LHS
            if (!lhsscan_->next()) {
                return false;
            }
        }
        // Reset RHS for new LHS row
        rhsscan_->before_first();
    }
    return false;
}

int32_t MultibufferProductScan::get_int(const std::string& fldname) {
    if (lhsscan_ && lhsscan_->has_field(fldname)) return lhsscan_->get_int(fldname);
    if (rhsscan_) return rhsscan_->get_int(fldname);
    throw std::runtime_error("MultibufferProductScan: field not found");
}

std::string MultibufferProductScan::get_string(const std::string& fldname) {
    if (lhsscan_ && lhsscan_->has_field(fldname)) return lhsscan_->get_string(fldname);
    if (rhsscan_) return rhsscan_->get_string(fldname);
    throw std::runtime_error("MultibufferProductScan: field not found");
}

Constant MultibufferProductScan::get_val(const std::string& fldname) {
    if (lhsscan_ && lhsscan_->has_field(fldname)) return lhsscan_->get_val(fldname);
    if (rhsscan_) return rhsscan_->get_val(fldname);
    throw std::runtime_error("MultibufferProductScan: field not found");
}

bool MultibufferProductScan::has_field(const std::string& fldname) const {
    return (lhsscan_ && lhsscan_->has_field(fldname)) ||
           (rhsscan_ && rhsscan_->has_field(fldname));
}

void MultibufferProductScan::close() {
    if (lhsscan_) lhsscan_->close();
    if (rhsscan_) rhsscan_->close();
}

bool MultibufferProductScan::use_next_chunk() {
    if (nextblknum_ >= filesize_) {
        return false;
    }
    if (rhsscan_) {
        rhsscan_->close();
    }
    size_t end = nextblknum_ + chunksize_ - 1;
    if (end >= filesize_) {
        end = filesize_ - 1;
    }
    rhsscan_ = std::make_unique<ChunkScan>(tx_, filename_, layout_, nextblknum_, end);
    lhsscan_->before_first();
    nextblknum_ = end + 1;
    return true;
}

} // namespace multibuffer
