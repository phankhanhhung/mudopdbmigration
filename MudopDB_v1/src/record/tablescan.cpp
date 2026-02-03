#include "record/tablescan.hpp"

namespace record {

TableScan::TableScan(std::shared_ptr<buffer::BufferMgr> bm,
                     const std::string& tablename,
                     const Layout& layout)
    : bm_(bm), layout_(layout), filename_(tablename + ".tbl"),
      currentslot_(std::nullopt), current_buffer_idx_(std::nullopt) {

    // If table file has blocks, move to first block
    // Otherwise, create the first block
    if (bm_->file_mgr()->length(filename_) == 0) {
        move_to_new_block();
    } else {
        move_to_block(0);
    }
}

void TableScan::before_first() {
    move_to_block(0);
}

bool TableScan::next() {
    currentslot_ = rp_->next_after(currentslot_);

    while (!currentslot_.has_value()) {
        if (at_last_block()) {
            return false;
        }
        move_to_block(rp_->block().number() + 1);
        currentslot_ = rp_->next_after(currentslot_);
    }
    return true;
}

int TableScan::get_int(const std::string& fldname) {
    return rp_->get_int(currentslot_.value(), fldname);
}

std::string TableScan::get_string(const std::string& fldname) {
    return rp_->get_string(currentslot_.value(), fldname);
}

Constant TableScan::get_val(const std::string& fldname) {
    if (layout_.schema()->type(fldname) == Type::INTEGER) {
        return Constant::with_int(get_int(fldname));
    } else {
        return Constant::with_string(get_string(fldname));
    }
}

bool TableScan::has_field(const std::string& fldname) const {
    return layout_.schema()->has_field(fldname);
}

void TableScan::close() {
    if (current_buffer_idx_.has_value()) {
        bm_->unpin(current_buffer_idx_.value());
        current_buffer_idx_ = std::nullopt;
    }
}

void TableScan::set_val(const std::string& fldname, const Constant& val) {
    if (layout_.schema()->type(fldname) == Type::INTEGER) {
        set_int(fldname, val.as_int().value());
    } else {
        set_string(fldname, val.as_string().value());
    }
}

void TableScan::set_int(const std::string& fldname, int32_t val) {
    rp_->set_int(currentslot_.value(), fldname, val);
}

void TableScan::set_string(const std::string& fldname, const std::string& val) {
    rp_->set_string(currentslot_.value(), fldname, val);
}

void TableScan::insert() {
    currentslot_ = rp_->insert_after(currentslot_);

    while (!currentslot_.has_value()) {
        if (at_last_block()) {
            move_to_new_block();
        } else {
            move_to_block(rp_->block().number() + 1);
        }
        currentslot_ = rp_->insert_after(currentslot_);
    }
}

void TableScan::delete_record() {
    rp_->delete_record(currentslot_.value());
}

std::optional<RID> TableScan::get_rid() const {
    if (!currentslot_.has_value()) {
        return std::nullopt;
    }
    return RID(rp_->block().number(), currentslot_.value());
}

void TableScan::move_to_rid(const RID& rid) {
    close();
    file::BlockId blk(filename_, rid.block_number());
    current_buffer_idx_ = bm_->pin(blk);
    rp_ = std::make_unique<RecordPage>(bm_->buffer(current_buffer_idx_.value()), layout_);
    currentslot_ = rid.slot();
}

void TableScan::move_to_block(int32_t blknum) {
    close();

    file::BlockId blk(filename_, blknum);
    current_buffer_idx_ = bm_->pin(blk);
    rp_ = std::make_unique<RecordPage>(bm_->buffer(current_buffer_idx_.value()), layout_);
    currentslot_ = std::nullopt;
}

void TableScan::move_to_new_block() {
    close();

    file::BlockId blk = bm_->file_mgr()->append(filename_);
    current_buffer_idx_ = bm_->pin(blk);
    rp_ = std::make_unique<RecordPage>(bm_->buffer(current_buffer_idx_.value()), layout_);
    rp_->format();
    currentslot_ = std::nullopt;
}

bool TableScan::at_last_block() const {
    return rp_->block().number() == bm_->file_mgr()->length(filename_) - 1;
}

} // namespace record
