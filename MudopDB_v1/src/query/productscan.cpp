#include "query/productscan.hpp"

ProductScan::ProductScan(std::unique_ptr<Scan> s1, std::unique_ptr<Scan> s2)
    : s1_(std::move(s1)), s2_(std::move(s2)) {
    before_first();
}

DbResult<void> ProductScan::before_first() {
    try {
        s1_->before_first().value();
        s1_->next().value();
        s2_->before_first().value();
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}

DbResult<bool> ProductScan::next() {
    try {
        if (s2_->next().value()) {
            return DbResult<bool>::ok(true);
        }
        s2_->before_first().value();
        return DbResult<bool>::ok(s2_->next().value() && s1_->next().value());
    } catch (const std::exception& e) {
        return DbResult<bool>::err(e.what());
    }
}

DbResult<int> ProductScan::get_int(const std::string& fldname) {
    if (s1_->has_field(fldname)) {
        return s1_->get_int(fldname);
    }
    return s2_->get_int(fldname);
}

DbResult<std::string> ProductScan::get_string(const std::string& fldname) {
    if (s1_->has_field(fldname)) {
        return s1_->get_string(fldname);
    }
    return s2_->get_string(fldname);
}

DbResult<Constant> ProductScan::get_val(const std::string& fldname) {
    if (s1_->has_field(fldname)) {
        return s1_->get_val(fldname);
    }
    return s2_->get_val(fldname);
}

bool ProductScan::has_field(const std::string& fldname) const {
    return s1_->has_field(fldname) || s2_->has_field(fldname);
}

DbResult<void> ProductScan::close() {
    try {
        s1_->close().value();
        s2_->close().value();
        return DbResult<void>::ok();
    } catch (const std::exception& e) {
        return DbResult<void>::err(e.what());
    }
}
