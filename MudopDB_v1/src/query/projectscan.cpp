#include "query/projectscan.hpp"
#include <algorithm>
#include <stdexcept>

ProjectScan::ProjectScan(std::unique_ptr<Scan> s, const std::vector<std::string>& fieldlist)
    : s_(std::move(s)), fieldlist_(fieldlist) {}

void ProjectScan::before_first() {
    s_->before_first();
}

bool ProjectScan::next() {
    return s_->next();
}

int ProjectScan::get_int(const std::string& fldname) {
    if (has_field(fldname)) {
        return s_->get_int(fldname);
    }
    throw std::runtime_error("ProjectScan: field " + fldname + " not found");
}

std::string ProjectScan::get_string(const std::string& fldname) {
    if (has_field(fldname)) {
        return s_->get_string(fldname);
    }
    throw std::runtime_error("ProjectScan: field " + fldname + " not found");
}

Constant ProjectScan::get_val(const std::string& fldname) {
    if (has_field(fldname)) {
        return s_->get_val(fldname);
    }
    throw std::runtime_error("ProjectScan: field " + fldname + " not found");
}

bool ProjectScan::has_field(const std::string& fldname) const {
    return std::find(fieldlist_.begin(), fieldlist_.end(), fldname) != fieldlist_.end();
}

void ProjectScan::close() {
    s_->close();
}
