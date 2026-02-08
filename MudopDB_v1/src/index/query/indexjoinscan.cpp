#include "index/query/indexjoinscan.hpp"

namespace index {

IndexJoinScan::IndexJoinScan(std::unique_ptr<Scan> lhs,
                             std::unique_ptr<::Index> idx,
                             const std::string& joinfield,
                             std::unique_ptr<record::TableScan> rhs)
    : lhs_(std::move(lhs)), idx_(std::move(idx)),
      joinfield_(joinfield), rhs_(std::move(rhs)) {
    before_first();
}

DbResult<void> IndexJoinScan::before_first() {
    try {
        lhs_->before_first().value();
        lhs_->next().value();
        reset_index();
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

DbResult<bool> IndexJoinScan::next() {
    try {
        while (true) {
            if (idx_->next()) {
                auto rid = idx_->get_data_rid();
                rhs_->move_to_rid(rid).value();
                return DbResult<bool>::ok(true);
            }
            if (!lhs_->next().value()) {
                return DbResult<bool>::ok(false);
            }
            reset_index();
        }
    } catch (const std::exception& e) {
        return DbResult<bool>::err(e.what());
    }
}

DbResult<int> IndexJoinScan::get_int(const std::string& fldname) {
    if (rhs_->has_field(fldname)) {
        return rhs_->get_int(fldname);
    }
    return lhs_->get_int(fldname);
}

DbResult<std::string> IndexJoinScan::get_string(const std::string& fldname) {
    if (rhs_->has_field(fldname)) {
        return rhs_->get_string(fldname);
    }
    return lhs_->get_string(fldname);
}

DbResult<Constant> IndexJoinScan::get_val(const std::string& fldname) {
    if (rhs_->has_field(fldname)) {
        return rhs_->get_val(fldname);
    }
    return lhs_->get_val(fldname);
}

bool IndexJoinScan::has_field(const std::string& fldname) const {
    return rhs_->has_field(fldname) || lhs_->has_field(fldname);
}

DbResult<void> IndexJoinScan::close() {
    try {
        lhs_->close().value();
        idx_->close();
        rhs_->close().value();
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

void IndexJoinScan::reset_index() {
    Constant searchkey = lhs_->get_val(joinfield_).value();
    idx_->before_first(searchkey);
}

} // namespace index
