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

DbResult<void> MultibufferProductScan::before_first() {
    try {
        nextblknum_ = 0;
        use_next_chunk();
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

DbResult<bool> MultibufferProductScan::next() {
    try {
        while (rhsscan_) {
            if (rhsscan_->next().value()) {
                return DbResult<bool>::ok(true);
            }
            if (!lhsscan_->next().value()) {
                if (!use_next_chunk()) {
                    return DbResult<bool>::ok(false);
                }
                if (!lhsscan_->next().value()) {
                    return DbResult<bool>::ok(false);
                }
            }
            rhsscan_->before_first().value();
        }
        return DbResult<bool>::ok(false);
    } catch (const std::exception& e) {
        return DbResult<bool>::err(e.what());
    }
}

DbResult<int> MultibufferProductScan::get_int(const std::string& fldname) {
    if (lhsscan_ && lhsscan_->has_field(fldname)) return lhsscan_->get_int(fldname);
    if (rhsscan_) return rhsscan_->get_int(fldname);
    return DbResult<int>::err("MultibufferProductScan: field not found");
}

DbResult<std::string> MultibufferProductScan::get_string(const std::string& fldname) {
    if (lhsscan_ && lhsscan_->has_field(fldname)) return lhsscan_->get_string(fldname);
    if (rhsscan_) return rhsscan_->get_string(fldname);
    return DbResult<std::string>::err("MultibufferProductScan: field not found");
}

DbResult<Constant> MultibufferProductScan::get_val(const std::string& fldname) {
    if (lhsscan_ && lhsscan_->has_field(fldname)) return lhsscan_->get_val(fldname);
    if (rhsscan_) return rhsscan_->get_val(fldname);
    return DbResult<Constant>::err("MultibufferProductScan: field not found");
}

bool MultibufferProductScan::has_field(const std::string& fldname) const {
    return (lhsscan_ && lhsscan_->has_field(fldname)) ||
           (rhsscan_ && rhsscan_->has_field(fldname));
}

DbResult<void> MultibufferProductScan::close() {
    try {
        if (lhsscan_) lhsscan_->close().value();
        if (rhsscan_) rhsscan_->close().value();
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

bool MultibufferProductScan::use_next_chunk() {
    if (nextblknum_ >= filesize_) {
        return false;
    }
    if (rhsscan_) {
        rhsscan_->close().value();
    }
    size_t end = nextblknum_ + chunksize_ - 1;
    if (end >= filesize_) {
        end = filesize_ - 1;
    }
    rhsscan_ = std::make_unique<ChunkScan>(tx_, filename_, layout_, nextblknum_, end);
    lhsscan_->before_first().value();
    nextblknum_ = end + 1;
    return true;
}

} // namespace multibuffer
