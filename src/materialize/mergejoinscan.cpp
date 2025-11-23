/**
 * MergeJoinScan implementation.
 * Sort-merge join of two sorted scans on a common field.
 */

#include "materialize/mergejoinscan.hpp"

namespace materialize {

MergeJoinScan::MergeJoinScan(std::unique_ptr<Scan> s1, std::unique_ptr<SortScan> s2,
                              const std::string& fldname1, const std::string& fldname2)
    : s1_(std::move(s1)), s2_(std::move(s2)),
      fldname1_(fldname1), fldname2_(fldname2) {}

void MergeJoinScan::before_first() {
    s1_->before_first();
    s2_->before_first();
}

bool MergeJoinScan::next() {
    bool hasmore2 = s2_->next();
    if (joinval_.has_value()) {
        if (hasmore2 && s2_->get_val(fldname2_) == joinval_.value()) {
            return true;
        }
    }

    bool hasmore1 = s1_->next();
    if (joinval_.has_value()) {
        if (hasmore1 && s1_->get_val(fldname1_) == joinval_.value()) {
            s2_->restore_position();
            return true;
        }
    }

    while (hasmore1 && hasmore2) {
        Constant v1 = s1_->get_val(fldname1_);
        Constant v2 = s2_->get_val(fldname2_);
        if (v1 < v2) {
            hasmore1 = s1_->next();
        } else if (v1 > v2) {
            hasmore2 = s2_->next();
        } else {
            s2_->save_position();
            joinval_ = s2_->get_val(fldname2_);
            return true;
        }
    }
    return false;
}

int MergeJoinScan::get_int(const std::string& fldname) {
    if (s1_->has_field(fldname)) return s1_->get_int(fldname);
    return s2_->get_int(fldname);
}

std::string MergeJoinScan::get_string(const std::string& fldname) {
    if (s1_->has_field(fldname)) return s1_->get_string(fldname);
    return s2_->get_string(fldname);
}

Constant MergeJoinScan::get_val(const std::string& fldname) {
    if (s1_->has_field(fldname)) return s1_->get_val(fldname);
    return s2_->get_val(fldname);
}

bool MergeJoinScan::has_field(const std::string& fldname) const {
    return s1_->has_field(fldname) || s2_->has_field(fldname);
}

void MergeJoinScan::close() {
    s1_->close();
    s2_->close();
}

} // namespace materialize
