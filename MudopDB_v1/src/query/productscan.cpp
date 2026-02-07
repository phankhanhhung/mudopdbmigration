#include "query/productscan.hpp"

ProductScan::ProductScan(std::unique_ptr<Scan> s1, std::unique_ptr<Scan> s2)
    : s1_(std::move(s1)), s2_(std::move(s2)) {
    before_first();
}

void ProductScan::before_first() {
    s1_->before_first();
    s1_->next();
    s2_->before_first();
}

bool ProductScan::next() {
    if (s2_->next()) {
        return true;
    }
    s2_->before_first();
    return s2_->next() && s1_->next();
}

int ProductScan::get_int(const std::string& fldname) {
    if (s1_->has_field(fldname)) {
        return s1_->get_int(fldname);
    }
    return s2_->get_int(fldname);
}

std::string ProductScan::get_string(const std::string& fldname) {
    if (s1_->has_field(fldname)) {
        return s1_->get_string(fldname);
    }
    return s2_->get_string(fldname);
}

Constant ProductScan::get_val(const std::string& fldname) {
    if (s1_->has_field(fldname)) {
        return s1_->get_val(fldname);
    }
    return s2_->get_val(fldname);
}

bool ProductScan::has_field(const std::string& fldname) const {
    return s1_->has_field(fldname) || s2_->has_field(fldname);
}

void ProductScan::close() {
    s1_->close();
    s2_->close();
}
