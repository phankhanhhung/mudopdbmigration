/**
 * RecordPage implementation.
 * Manages fixed-length record slots within a single disk block.
 * Each slot has a used/empty flag followed by the field data.
 */

#include "record/recordpage.hpp"
#include "tx/transaction.hpp"

namespace record {

RecordPage::RecordPage(std::shared_ptr<tx::Transaction> tx,
                       const file::BlockId& blk,
                       const Layout& layout)
    : tx_(tx), blk_(blk), layout_(layout) {
    tx_->pin(blk_);
}

int32_t RecordPage::get_int(size_t slot, const std::string& fldname) {
    size_t fldpos = offset(slot) + layout_.offset(fldname);
    return tx_->get_int(blk_, fldpos);
}

std::string RecordPage::get_string(size_t slot, const std::string& fldname) {
    size_t fldpos = offset(slot) + layout_.offset(fldname);
    return tx_->get_string(blk_, fldpos);
}

void RecordPage::set_int(size_t slot, const std::string& fldname, int32_t val) {
    size_t fldpos = offset(slot) + layout_.offset(fldname);
    tx_->set_int(blk_, fldpos, val, true);
}

void RecordPage::set_string(size_t slot, const std::string& fldname, const std::string& val) {
    size_t fldpos = offset(slot) + layout_.offset(fldname);
    tx_->set_string(blk_, fldpos, val, true);
}

void RecordPage::delete_record(size_t slot) {
    set_flag(slot, Flag::EMPTY);
}

void RecordPage::format() {
    size_t slot = 0;
    while (is_valid_slot(slot)) {
        tx_->set_int(blk_, offset(slot), static_cast<int32_t>(Flag::EMPTY), false);

        for (const auto& fldname : layout_.schema()->fields()) {
            size_t fldpos = offset(slot) + layout_.offset(fldname);
            if (layout_.schema()->type(fldname) == Type::INTEGER) {
                tx_->set_int(blk_, fldpos, 0, false);
            } else {
                tx_->set_string(blk_, fldpos, "", false);
            }
        }
        slot++;
    }
}

std::optional<size_t> RecordPage::next_after(std::optional<size_t> slot) {
    return search_after(slot, Flag::USED);
}

std::optional<size_t> RecordPage::insert_after(std::optional<size_t> slot) {
    std::optional<size_t> newslot = search_after(slot, Flag::EMPTY);
    if (newslot.has_value()) {
        set_flag(newslot.value(), Flag::USED);
    }
    return newslot;
}

const file::BlockId& RecordPage::block() const {
    return blk_;
}

void RecordPage::set_flag(size_t slot, Flag flag) {
    tx_->set_int(blk_, offset(slot), static_cast<int32_t>(flag), true);
}

std::optional<size_t> RecordPage::search_after(std::optional<size_t> slot, Flag flag) {
    size_t current = slot.has_value() ? slot.value() + 1 : 0;
    int32_t f = static_cast<int32_t>(flag);

    while (is_valid_slot(current)) {
        if (tx_->get_int(blk_, offset(current)) == f) {
            return current;
        }
        current++;
    }

    return std::nullopt;
}

bool RecordPage::is_valid_slot(size_t slot) const {
    return offset(slot + 1) <= tx_->block_size();
}

size_t RecordPage::offset(size_t slot) const {
    return slot * layout_.slot_size();
}

} // namespace record
