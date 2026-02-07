#include "materialize/sortscan.hpp"
#include <stdexcept>

namespace materialize {

SortScan::SortScan(const std::vector<std::shared_ptr<TempTable>>& runs,
                   const RecordComparator& comp)
    : comp_(comp), currentidx_(std::nullopt),
      hasmore1_(false), hasmore2_(false),
      savedposition_{std::nullopt, std::nullopt} {
    if (!runs.empty()) {
        s1_ = runs[0]->open();
        hasmore1_ = s1_->next();
    }
    if (runs.size() > 1) {
        s2_ = runs[1]->open();
        hasmore2_ = s2_->next();
    }
}

void SortScan::before_first() {
    currentidx_ = std::nullopt;
    s1_->before_first();
    hasmore1_ = s1_->next();
    if (s2_) {
        s2_->before_first();
        hasmore2_ = s2_->next();
    }
}

bool SortScan::next() {
    if (currentidx_.has_value()) {
        if (currentidx_.value() == 1) {
            hasmore1_ = s1_->next();
        } else if (currentidx_.value() == 2 && s2_) {
            hasmore2_ = s2_->next();
        }
    }

    if (!hasmore1_ && !hasmore2_) {
        return false;
    }
    if (hasmore1_ && hasmore2_) {
        if (comp_.compare(*s1_, *s2_) < 0) {
            currentidx_ = 1;
        } else {
            currentidx_ = 2;
        }
    } else if (hasmore1_) {
        currentidx_ = 1;
    } else {
        currentidx_ = 2;
    }
    return true;
}

void SortScan::close() {
    s1_->close();
    if (s2_) {
        s2_->close();
    }
}

record::TableScan& SortScan::current_scan() {
    if (!currentidx_.has_value()) {
        throw std::runtime_error("SortScan: no current scan");
    }
    if (currentidx_.value() == 1) return *s1_;
    if (currentidx_.value() == 2 && s2_) return *s2_;
    throw std::runtime_error("SortScan: invalid scan index");
}

int SortScan::get_int(const std::string& fldname) {
    return current_scan().get_int(fldname);
}

std::string SortScan::get_string(const std::string& fldname) {
    return current_scan().get_string(fldname);
}

Constant SortScan::get_val(const std::string& fldname) {
    return current_scan().get_val(fldname);
}

bool SortScan::has_field(const std::string& fldname) const {
    return s1_->has_field(fldname);
}

void SortScan::save_position() {
    savedposition_[0] = s1_->get_rid();
    if (s2_) {
        savedposition_[1] = s2_->get_rid();
    }
}

void SortScan::restore_position() {
    if (savedposition_[0].has_value()) {
        s1_->move_to_rid(savedposition_[0].value());
    }
    if (savedposition_[1].has_value() && s2_) {
        s2_->move_to_rid(savedposition_[1].value());
    }
}

} // namespace materialize
