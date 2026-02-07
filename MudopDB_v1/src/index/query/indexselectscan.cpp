#include "index/query/indexselectscan.hpp"

namespace index {

IndexSelectScan::IndexSelectScan(std::unique_ptr<record::TableScan> ts,
                                 std::unique_ptr<::Index> idx,
                                 const Constant& val)
    : ts_(std::move(ts)), idx_(std::move(idx)), val_(val) {
    before_first();
}

void IndexSelectScan::before_first() {
    idx_->before_first(val_);
}

bool IndexSelectScan::next() {
    bool ok = idx_->next();
    if (ok) {
        auto rid = idx_->get_data_rid();
        ts_->move_to_rid(rid);
    }
    return ok;
}

int32_t IndexSelectScan::get_int(const std::string& fldname) {
    return ts_->get_int(fldname);
}

std::string IndexSelectScan::get_string(const std::string& fldname) {
    return ts_->get_string(fldname);
}

Constant IndexSelectScan::get_val(const std::string& fldname) {
    return ts_->get_val(fldname);
}

bool IndexSelectScan::has_field(const std::string& fldname) const {
    return ts_->has_field(fldname);
}

void IndexSelectScan::close() {
    idx_->close();
    ts_->close();
}

} // namespace index
