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

DbResult<void> GroupByScan::before_first() {
    try {
        s_->before_first().value();
        moregroups_ = s_->next().value();
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

DbResult<bool> GroupByScan::next() {
    try {
        if (!moregroups_) return DbResult<bool>::ok(false);

        for (auto& f : aggfns_) {
            f->process_first(*s_);
        }
        groupval_ = GroupValue(*s_, groupfields_);

        while (s_->next().value()) {
            moregroups_ = true;
            GroupValue gv(*s_, groupfields_);
            if (groupval_.has_value() && groupval_.value() != gv) {
                return DbResult<bool>::ok(true);
            }
            for (auto& f : aggfns_) {
                f->process_next(*s_);
            }
        }
        moregroups_ = false;
        return DbResult<bool>::ok(true);
    } catch (const std::exception& e) {
        return DbResult<bool>::err(e.what());
    }
}

DbResult<void> GroupByScan::close() {
    return s_->close();
}

DbResult<Constant> GroupByScan::get_val(const std::string& fldname) {
    try {
        if (std::find(groupfields_.begin(), groupfields_.end(), fldname) != groupfields_.end()) {
            if (groupval_.has_value()) {
                auto v = groupval_->get_val(fldname);
                if (v.has_value()) return DbResult<Constant>::ok(v.value());
            }
            return DbResult<Constant>::err("GroupByScan: field not found in group");
        }
        for (const auto& f : aggfns_) {
            if (f->field_name() == fldname) {
                auto v = f->value();
                if (v.has_value()) return DbResult<Constant>::ok(v.value());
                return DbResult<Constant>::err("GroupByScan: no value for aggregate");
            }
        }
        return DbResult<Constant>::err("GroupByScan: field " + fldname + " not found");
    } catch (const std::exception& e) {
        return DbResult<Constant>::err(e.what());
    }
}

DbResult<int> GroupByScan::get_int(const std::string& fldname) {
    try {
        Constant v = get_val(fldname).value();
        auto i = v.as_int();
        if (i.has_value()) return DbResult<int>::ok(i.value());
        return DbResult<int>::err("GroupByScan: not an int");
    } catch (const std::exception& e) {
        return DbResult<int>::err(e.what());
    }
}

DbResult<std::string> GroupByScan::get_string(const std::string& fldname) {
    try {
        Constant v = get_val(fldname).value();
        auto s = v.as_string();
        if (s.has_value()) return DbResult<std::string>::ok(s.value());
        return DbResult<std::string>::err("GroupByScan: not a string");
    } catch (const std::exception& e) {
        return DbResult<std::string>::err(e.what());
    }
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
