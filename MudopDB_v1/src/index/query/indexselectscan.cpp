#include "index/query/indexselectscan.hpp"

namespace index {

IndexSelectScan::IndexSelectScan(std::unique_ptr<record::TableScan> ts,
                                 std::unique_ptr<::Index> idx,
                                 const Constant& val)
    : ts_(std::move(ts)), idx_(std::move(idx)), val_(val) {
    before_first();
}

DbResult<void> IndexSelectScan::before_first() {
    try {
        idx_->before_first(val_);
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

DbResult<bool> IndexSelectScan::next() {
    try {
        bool ok = idx_->next();
        if (ok) {
            auto rid = idx_->get_data_rid();
            ts_->move_to_rid(rid).value();
        }
        return DbResult<bool>::ok(ok);
    } catch (const std::exception& e) {
        return DbResult<bool>::err(e.what());
    }
}

DbResult<int> IndexSelectScan::get_int(const std::string& fldname) {
    return ts_->get_int(fldname);
}

DbResult<std::string> IndexSelectScan::get_string(const std::string& fldname) {
    return ts_->get_string(fldname);
}

DbResult<Constant> IndexSelectScan::get_val(const std::string& fldname) {
    return ts_->get_val(fldname);
}

bool IndexSelectScan::has_field(const std::string& fldname) const {
    return ts_->has_field(fldname);
}

DbResult<void> IndexSelectScan::close() {
    try {
        idx_->close();
        ts_->close().value();
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

} // namespace index
