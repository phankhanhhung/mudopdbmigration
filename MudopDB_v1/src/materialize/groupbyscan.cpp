#include "materialize/groupbyscan.hpp"
#include <stdexcept>
#include <algorithm>

namespace materialize {

GroupByScan::GroupByScan(std::unique_ptr<Scan> s,
                         const std::vector<std::string>& groupfields,
                         std::vector<std::unique_ptr<AggregationFn>> aggfns)
    : s_(std::move(s)), groupfields_(groupfields),
      aggfns_(std::move(aggfns)), moregroups_(false) {
    before_first();
}

void GroupByScan::before_first() {
    s_->before_first();
    moregroups_ = s_->next();
}

bool GroupByScan::next() {
    if (!moregroups_) return false;

    for (auto& f : aggfns_) {
        f->process_first(*s_);
    }
    groupval_ = GroupValue(*s_, groupfields_);

    while (s_->next()) {
        moregroups_ = true;
        GroupValue gv(*s_, groupfields_);
        if (groupval_.has_value() && groupval_.value() != gv) {
            return true;
        }
        for (auto& f : aggfns_) {
            f->process_next(*s_);
        }
    }
    moregroups_ = false;
    return true;
}

void GroupByScan::close() {
    s_->close();
}

Constant GroupByScan::get_val(const std::string& fldname) {
    if (std::find(groupfields_.begin(), groupfields_.end(), fldname) != groupfields_.end()) {
        if (groupval_.has_value()) {
            auto v = groupval_->get_val(fldname);
            if (v.has_value()) return v.value();
        }
        throw std::runtime_error("GroupByScan: field not found in group");
    }
    for (const auto& f : aggfns_) {
        if (f->field_name() == fldname) {
            auto v = f->value();
            if (v.has_value()) return v.value();
            throw std::runtime_error("GroupByScan: no value for aggregate");
        }
    }
    throw std::runtime_error("GroupByScan: field " + fldname + " not found");
}

int GroupByScan::get_int(const std::string& fldname) {
    Constant v = get_val(fldname);
    auto i = v.as_int();
    if (i.has_value()) return i.value();
    throw std::runtime_error("GroupByScan: not an int");
}

std::string GroupByScan::get_string(const std::string& fldname) {
    Constant v = get_val(fldname);
    auto s = v.as_string();
    if (s.has_value()) return s.value();
    throw std::runtime_error("GroupByScan: not a string");
}

bool GroupByScan::has_field(const std::string& fldname) const {
    if (std::find(groupfields_.begin(), groupfields_.end(), fldname) != groupfields_.end()) {
        return true;
    }
    for (const auto& f : aggfns_) {
        if (f->field_name() == fldname) return true;
    }
    return false;
}

} // namespace materialize
