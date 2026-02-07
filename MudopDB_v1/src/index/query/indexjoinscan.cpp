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

void IndexJoinScan::before_first() {
    lhs_->before_first();
    lhs_->next();
    reset_index();
}

bool IndexJoinScan::next() {
    while (true) {
        if (idx_->next()) {
            auto rid = idx_->get_data_rid();
            rhs_->move_to_rid(rid);
            return true;
        }
        if (!lhs_->next()) {
            return false;
        }
        reset_index();
    }
}

int32_t IndexJoinScan::get_int(const std::string& fldname) {
    if (rhs_->has_field(fldname)) {
        return rhs_->get_int(fldname);
    }
    return lhs_->get_int(fldname);
}

std::string IndexJoinScan::get_string(const std::string& fldname) {
    if (rhs_->has_field(fldname)) {
        return rhs_->get_string(fldname);
    }
    return lhs_->get_string(fldname);
}

Constant IndexJoinScan::get_val(const std::string& fldname) {
    if (rhs_->has_field(fldname)) {
        return rhs_->get_val(fldname);
    }
    return lhs_->get_val(fldname);
}

bool IndexJoinScan::has_field(const std::string& fldname) const {
    return rhs_->has_field(fldname) || lhs_->has_field(fldname);
}

void IndexJoinScan::close() {
    lhs_->close();
    idx_->close();
    rhs_->close();
}

void IndexJoinScan::reset_index() {
    Constant searchkey = lhs_->get_val(joinfield_);
    idx_->before_first(searchkey);
}

} // namespace index
