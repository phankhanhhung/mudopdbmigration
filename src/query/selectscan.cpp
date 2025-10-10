/**
 * SelectScan implementation.
 * Filters records from an underlying scan using a Predicate.
 */

#include "query/selectscan.hpp"
#include <stdexcept>

SelectScan::SelectScan(std::unique_ptr<Scan> s, const Predicate& pred)
    : s_(std::move(s)), pred_(pred) {}

void SelectScan::before_first() {
    s_->before_first();
}

bool SelectScan::next() {
    while (s_->next()) {
        if (pred_.is_satisfied(*s_)) {
            return true;
        }
    }
    return false;
}

int SelectScan::get_int(const std::string& fldname) {
    return s_->get_int(fldname);
}

std::string SelectScan::get_string(const std::string& fldname) {
    return s_->get_string(fldname);
}

Constant SelectScan::get_val(const std::string& fldname) {
    return s_->get_val(fldname);
}

bool SelectScan::has_field(const std::string& fldname) const {
    return s_->has_field(fldname);
}

void SelectScan::close() {
    s_->close();
}

UpdateScan* SelectScan::get_update_scan() const {
    return dynamic_cast<UpdateScan*>(s_.get());
}

void SelectScan::set_val(const std::string& fldname, const Constant& val) {
    auto us = get_update_scan();
    if (us) us->set_val(fldname, val);
}

void SelectScan::set_int(const std::string& fldname, int32_t val) {
    auto us = get_update_scan();
    if (us) us->set_int(fldname, val);
}

void SelectScan::set_string(const std::string& fldname, const std::string& val) {
    auto us = get_update_scan();
    if (us) us->set_string(fldname, val);
}

void SelectScan::insert() {
    auto us = get_update_scan();
    if (us) us->insert();
}

void SelectScan::delete_record() {
    auto us = get_update_scan();
    if (us) us->delete_record();
}

std::optional<record::RID> SelectScan::get_rid() const {
    auto us = get_update_scan();
    if (us) return us->get_rid();
    return std::nullopt;
}

void SelectScan::move_to_rid(const record::RID& rid) {
    auto us = get_update_scan();
    if (us) us->move_to_rid(rid);
}
