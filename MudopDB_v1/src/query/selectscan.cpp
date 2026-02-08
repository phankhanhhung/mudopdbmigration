#include "query/selectscan.hpp"
#include <stdexcept>

SelectScan::SelectScan(std::unique_ptr<Scan> s, const Predicate& pred)
    : s_(std::move(s)), pred_(pred) {}

DbResult<void> SelectScan::before_first() {
    return s_->before_first();
}

DbResult<bool> SelectScan::next() {
    try {
        while (s_->next().value()) {
            if (pred_.is_satisfied(*s_)) {
                return DbResult<bool>::ok(true);
            }
        }
        return DbResult<bool>::ok(false);
    } catch (const std::exception& e) {
        return DbResult<bool>::err(e.what());
    }
}

DbResult<int> SelectScan::get_int(const std::string& fldname) {
    return s_->get_int(fldname);
}

DbResult<std::string> SelectScan::get_string(const std::string& fldname) {
    return s_->get_string(fldname);
}

DbResult<Constant> SelectScan::get_val(const std::string& fldname) {
    return s_->get_val(fldname);
}

bool SelectScan::has_field(const std::string& fldname) const {
    return s_->has_field(fldname);
}

DbResult<void> SelectScan::close() {
    return s_->close();
}

UpdateScan* SelectScan::get_update_scan() const {
    return dynamic_cast<UpdateScan*>(s_.get());
}

DbResult<void> SelectScan::set_val(const std::string& fldname, const Constant& val) {
    auto us = get_update_scan();
    if (us) return us->set_val(fldname, val);
    return DbResult<void>::ok();
}

DbResult<void> SelectScan::set_int(const std::string& fldname, int32_t val) {
    auto us = get_update_scan();
    if (us) return us->set_int(fldname, val);
    return DbResult<void>::ok();
}

DbResult<void> SelectScan::set_string(const std::string& fldname, const std::string& val) {
    auto us = get_update_scan();
    if (us) return us->set_string(fldname, val);
    return DbResult<void>::ok();
}

DbResult<void> SelectScan::insert() {
    auto us = get_update_scan();
    if (us) return us->insert();
    return DbResult<void>::ok();
}

DbResult<void> SelectScan::delete_record() {
    auto us = get_update_scan();
    if (us) return us->delete_record();
    return DbResult<void>::ok();
}

std::optional<record::RID> SelectScan::get_rid() const {
    auto us = get_update_scan();
    if (us) return us->get_rid();
    return std::nullopt;
}

DbResult<void> SelectScan::move_to_rid(const record::RID& rid) {
    auto us = get_update_scan();
    if (us) return us->move_to_rid(rid);
    return DbResult<void>::ok();
}
