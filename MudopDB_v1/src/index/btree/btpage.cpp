#include "index/btree/btpage.hpp"
#include "tx/transaction.hpp"
#include "record/schema.hpp"
#include <stdexcept>

namespace index {

BTPage::BTPage(std::shared_ptr<tx::Transaction> tx,
               const file::BlockId& currentblk,
               const record::Layout& layout)
    : tx_(tx), currentblk_(currentblk), layout_(layout) {
    tx_->pin(currentblk);
}

int32_t BTPage::find_slot_before(const Constant& searchkey) {
    int32_t slot = 0;
    while (static_cast<size_t>(slot) < get_num_recs() && get_data_val(slot) < searchkey) {
        slot++;
    }
    return slot - 1;
}

void BTPage::close() {
    if (currentblk_.has_value()) {
        tx_->unpin(currentblk_.value());
    }
    currentblk_.reset();
}

bool BTPage::is_full() const {
    return slotpos(get_num_recs() + 1) >= tx_->block_size();
}

file::BlockId BTPage::split(size_t splitpos, int32_t flag) {
    auto newblk = append_new(flag);
    BTPage newpage(tx_, newblk, layout_);
    transfer_recs(splitpos, newpage);
    newpage.set_flag(flag);
    newpage.close();
    return newblk;
}

Constant BTPage::get_data_val(size_t slot) const {
    return get_val(slot, "dataval");
}

int32_t BTPage::get_flag() const {
    if (currentblk_.has_value()) {
        return tx_->get_int(currentblk_.value(), 0);
    }
    throw std::runtime_error("BTPage: no current block");
}

void BTPage::set_flag(int32_t val) {
    if (currentblk_.has_value()) {
        tx_->set_int(currentblk_.value(), 0, val, true);
        return;
    }
    throw std::runtime_error("BTPage: no current block");
}

file::BlockId BTPage::append_new(int32_t flag) {
    if (currentblk_.has_value()) {
        auto blk = tx_->append(currentblk_.value().file_name());
        tx_->pin(blk);
        format(blk, flag);
        return blk;
    }
    throw std::runtime_error("BTPage: no current block");
}

void BTPage::format(const file::BlockId& blk, int32_t flag) {
    tx_->set_int(blk, 0, flag, false);
    size_t bytes = 4;
    tx_->set_int(blk, bytes, 0, false);
    size_t recsize = layout_.slot_size();
    size_t pos = 2 * bytes;
    while (pos + recsize <= tx_->block_size()) {
        make_default_record(blk, pos);
        pos += recsize;
    }
}

void BTPage::make_default_record(const file::BlockId& blk, size_t pos) {
    auto sch = layout_.schema();
    for (const auto& fldname : sch->fields()) {
        size_t offset = layout_.offset(fldname);
        if (sch->type(fldname) == record::Type::INTEGER) {
            tx_->set_int(blk, pos + offset, 0, false);
        } else {
            tx_->set_string(blk, pos + offset, "", false);
        }
    }
}

int32_t BTPage::get_child_num(size_t slot) const {
    return get_int(slot, "block");
}

void BTPage::insert_dir(size_t slot, const Constant& val, int32_t blknum) {
    insert(slot);
    set_val(slot, "dataval", val);
    set_int(slot, "block", blknum);
}

record::RID BTPage::get_data_rid(size_t slot) const {
    return record::RID(get_int(slot, "block"), get_int(slot, "id"));
}

void BTPage::insert_leaf(size_t slot, const Constant& val, const record::RID& rid) {
    insert(slot);
    set_val(slot, "dataval", val);
    set_int(slot, "block", rid.block_number());
    set_int(slot, "id", rid.slot());
}

void BTPage::delete_entry(size_t slot) {
    for (size_t i = slot + 1; i < get_num_recs(); i++) {
        copy_record(i, i - 1);
    }
    set_num_recs(get_num_recs() - 1);
}

size_t BTPage::get_num_recs() const {
    size_t bytes = 4;
    if (currentblk_.has_value()) {
        return static_cast<size_t>(tx_->get_int(currentblk_.value(), bytes));
    }
    throw std::runtime_error("BTPage: no current block");
}

int32_t BTPage::get_int(size_t slot, const std::string& fldname) const {
    size_t pos = fldpos(slot, fldname);
    if (currentblk_.has_value()) {
        return tx_->get_int(currentblk_.value(), pos);
    }
    throw std::runtime_error("BTPage: no current block");
}

std::string BTPage::get_string(size_t slot, const std::string& fldname) const {
    size_t pos = fldpos(slot, fldname);
    if (currentblk_.has_value()) {
        return tx_->get_string(currentblk_.value(), pos);
    }
    throw std::runtime_error("BTPage: no current block");
}

Constant BTPage::get_val(size_t slot, const std::string& fldname) const {
    auto sch = layout_.schema();
    if (sch->type(fldname) == record::Type::INTEGER) {
        return Constant::with_int(get_int(slot, fldname));
    }
    return Constant::with_string(get_string(slot, fldname));
}

void BTPage::set_int(size_t slot, const std::string& fldname, int32_t val) {
    size_t pos = fldpos(slot, fldname);
    if (currentblk_.has_value()) {
        tx_->set_int(currentblk_.value(), pos, val, true);
        return;
    }
    throw std::runtime_error("BTPage: no current block");
}

void BTPage::set_string(size_t slot, const std::string& fldname, const std::string& val) {
    size_t pos = fldpos(slot, fldname);
    if (currentblk_.has_value()) {
        tx_->set_string(currentblk_.value(), pos, val, true);
        return;
    }
    throw std::runtime_error("BTPage: no current block");
}

void BTPage::set_val(size_t slot, const std::string& fldname, const Constant& val) {
    auto sch = layout_.schema();
    if (sch->type(fldname) == record::Type::INTEGER) {
        if (auto v = val.as_int()) {
            set_int(slot, fldname, *v);
        }
    } else {
        if (auto v = val.as_string()) {
            set_string(slot, fldname, *v);
        }
    }
}

void BTPage::set_num_recs(size_t n) {
    size_t bytes = 4;
    if (currentblk_.has_value()) {
        tx_->set_int(currentblk_.value(), bytes, static_cast<int32_t>(n), true);
        return;
    }
    throw std::runtime_error("BTPage: no current block");
}

void BTPage::insert(size_t slot) {
    for (size_t i = get_num_recs(); i > slot; i--) {
        copy_record(i - 1, i);
    }
    set_num_recs(get_num_recs() + 1);
}

void BTPage::copy_record(size_t from, size_t to) {
    auto sch = layout_.schema();
    for (const auto& fldname : sch->fields()) {
        set_val(to, fldname, get_val(from, fldname));
    }
}

void BTPage::transfer_recs(size_t slot, BTPage& dest) {
    size_t destslot = 0;
    while (slot < get_num_recs()) {
        dest.insert(destslot);
        auto sch = layout_.schema();
        for (const auto& fldname : sch->fields()) {
            dest.set_val(destslot, fldname, get_val(slot, fldname));
        }
        delete_entry(slot);
        destslot++;
    }
}

size_t BTPage::fldpos(size_t slot, const std::string& fldname) const {
    size_t offset = layout_.offset(fldname);
    return slotpos(slot) + offset;
}

size_t BTPage::slotpos(size_t slot) const {
    size_t slotsize = layout_.slot_size();
    size_t bytes = 4;
    return bytes + bytes + (slot * slotsize);
}

} // namespace index
