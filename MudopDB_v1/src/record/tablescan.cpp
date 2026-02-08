#include "record/tablescan.hpp"
#include "tx/transaction.hpp"

namespace record {

TableScan::TableScan(std::shared_ptr<tx::Transaction> tx,
                     const std::string& tablename,
                     const Layout& layout)
    : tx_(tx), layout_(layout), filename_(tablename + ".tbl"),
      currentslot_(std::nullopt) {

    if (tx_->size(filename_) == 0) {
        move_to_new_block();
    } else {
        move_to_block(0);
    }
}

DbResult<void> TableScan::before_first() {
    try {
        move_to_block(0);
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

DbResult<bool> TableScan::next() {
    try {
        currentslot_ = rp_->next_after(currentslot_);

        while (!currentslot_.has_value()) {
            if (at_last_block()) {
                return DbResult<bool>::ok(false);
            }
            move_to_block(rp_->block().number() + 1);
            currentslot_ = rp_->next_after(currentslot_);
        }
        return DbResult<bool>::ok(true);
    } catch (const std::exception& e) {
        return DbResult<bool>::err(e.what());
    }
}

DbResult<int> TableScan::get_int(const std::string& fldname) {
    try {
        return DbResult<int>::ok(rp_->get_int(currentslot_.value(), fldname));
    } catch (const std::exception& e) {
        return DbResult<int>::err(e.what());
    }
}

DbResult<std::string> TableScan::get_string(const std::string& fldname) {
    try {
        return DbResult<std::string>::ok(rp_->get_string(currentslot_.value(), fldname));
    } catch (const std::exception& e) {
        return DbResult<std::string>::err(e.what());
    }
}

DbResult<Constant> TableScan::get_val(const std::string& fldname) {
    try {
        if (layout_.schema()->type(fldname) == Type::INTEGER) {
            return DbResult<Constant>::ok(Constant::with_int(get_int(fldname).value()));
        } else {
            return DbResult<Constant>::ok(Constant::with_string(get_string(fldname).value()));
        }
    } catch (const std::exception& e) {
        return DbResult<Constant>::err(e.what());
    }
}

bool TableScan::has_field(const std::string& fldname) const {
    return layout_.schema()->has_field(fldname);
}

DbResult<void> TableScan::close() {
    try {
        if (rp_) {
            tx_->unpin(rp_->block());
        }
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

DbResult<void> TableScan::set_val(const std::string& fldname, const Constant& val) {
    try {
        if (layout_.schema()->type(fldname) == Type::INTEGER) {
            set_int(fldname, val.as_int().value()).value();
        } else {
            set_string(fldname, val.as_string().value()).value();
        }
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

DbResult<void> TableScan::set_int(const std::string& fldname, int32_t val) {
    try {
        rp_->set_int(currentslot_.value(), fldname, val);
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

DbResult<void> TableScan::set_string(const std::string& fldname, const std::string& val) {
    try {
        rp_->set_string(currentslot_.value(), fldname, val);
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

DbResult<void> TableScan::insert() {
    try {
        currentslot_ = rp_->insert_after(currentslot_);

        while (!currentslot_.has_value()) {
            if (at_last_block()) {
                move_to_new_block();
            } else {
                move_to_block(rp_->block().number() + 1);
            }
            currentslot_ = rp_->insert_after(currentslot_);
        }
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

DbResult<void> TableScan::delete_record() {
    try {
        rp_->delete_record(currentslot_.value());
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

std::optional<RID> TableScan::get_rid() const {
    if (!currentslot_.has_value()) {
        return std::nullopt;
    }
    return RID(rp_->block().number(), currentslot_.value());
}

DbResult<void> TableScan::move_to_rid(const RID& rid) {
    try {
        close().value();
        file::BlockId blk(filename_, rid.block_number());
        rp_ = std::make_unique<RecordPage>(tx_, blk, layout_);
        currentslot_ = rid.slot();
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

void TableScan::move_to_block(int32_t blknum) {
    close().value();
    file::BlockId blk(filename_, blknum);
    rp_ = std::make_unique<RecordPage>(tx_, blk, layout_);
    currentslot_ = std::nullopt;
}

void TableScan::move_to_new_block() {
    close().value();
    file::BlockId blk = tx_->append(filename_);
    rp_ = std::make_unique<RecordPage>(tx_, blk, layout_);
    rp_->format();
    currentslot_ = std::nullopt;
}

bool TableScan::at_last_block() const {
    return rp_->block().number() == static_cast<int32_t>(tx_->size(filename_)) - 1;
}

} // namespace record
