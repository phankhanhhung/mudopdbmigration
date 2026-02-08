#include "materialize/mergejoinscan.hpp"

namespace materialize {

MergeJoinScan::MergeJoinScan(std::unique_ptr<Scan> s1, std::unique_ptr<SortScan> s2,
                              const std::string& fldname1, const std::string& fldname2)
    : s1_(std::move(s1)), s2_(std::move(s2)),
      fldname1_(fldname1), fldname2_(fldname2) {}

DbResult<void> MergeJoinScan::before_first() {
    try {
        s1_->before_first().value();
        s2_->before_first().value();
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

DbResult<bool> MergeJoinScan::next() {
    try {
        bool hasmore2 = s2_->next().value();
        if (joinval_.has_value()) {
            if (hasmore2 && s2_->get_val(fldname2_).value() == joinval_.value()) {
                return DbResult<bool>::ok(true);
            }
        }

        bool hasmore1 = s1_->next().value();
        if (joinval_.has_value()) {
            if (hasmore1 && s1_->get_val(fldname1_).value() == joinval_.value()) {
                s2_->restore_position();
                return DbResult<bool>::ok(true);
            }
        }

        while (hasmore1 && hasmore2) {
            Constant v1 = s1_->get_val(fldname1_).value();
            Constant v2 = s2_->get_val(fldname2_).value();
            if (v1 < v2) {
                hasmore1 = s1_->next().value();
            } else if (v1 > v2) {
                hasmore2 = s2_->next().value();
            } else {
                s2_->save_position();
                joinval_ = s2_->get_val(fldname2_).value();
                return DbResult<bool>::ok(true);
            }
        }
        return DbResult<bool>::ok(false);
    } catch (const std::exception& e) {
        return DbResult<bool>::err(e.what());
    }
}

DbResult<int> MergeJoinScan::get_int(const std::string& fldname) {
    if (s1_->has_field(fldname)) return s1_->get_int(fldname);
    return s2_->get_int(fldname);
}

DbResult<std::string> MergeJoinScan::get_string(const std::string& fldname) {
    if (s1_->has_field(fldname)) return s1_->get_string(fldname);
    return s2_->get_string(fldname);
}

DbResult<Constant> MergeJoinScan::get_val(const std::string& fldname) {
    if (s1_->has_field(fldname)) return s1_->get_val(fldname);
    return s2_->get_val(fldname);
}

bool MergeJoinScan::has_field(const std::string& fldname) const {
    return s1_->has_field(fldname) || s2_->has_field(fldname);
}

DbResult<void> MergeJoinScan::close() {
    try {
        s1_->close().value();
        s2_->close().value();
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

} // namespace materialize
