#include "query/projectscan.hpp"
#include <algorithm>
#include <stdexcept>

ProjectScan::ProjectScan(std::unique_ptr<Scan> s, const std::vector<std::string>& fieldlist)
    : s_(std::move(s)), fieldlist_(fieldlist) {}

DbResult<void> ProjectScan::before_first() {
    return s_->before_first();
}

DbResult<bool> ProjectScan::next() {
    return s_->next();
}

DbResult<int> ProjectScan::get_int(const std::string& fldname) {
    if (has_field(fldname)) {
        return s_->get_int(fldname);
    }
    return DbResult<int>::err("ProjectScan: field " + fldname + " not found");
}

DbResult<std::string> ProjectScan::get_string(const std::string& fldname) {
    if (has_field(fldname)) {
        return s_->get_string(fldname);
    }
    return DbResult<std::string>::err("ProjectScan: field " + fldname + " not found");
}

DbResult<Constant> ProjectScan::get_val(const std::string& fldname) {
    if (has_field(fldname)) {
        return s_->get_val(fldname);
    }
    return DbResult<Constant>::err("ProjectScan: field " + fldname + " not found");
}

bool ProjectScan::has_field(const std::string& fldname) const {
    return std::find(fieldlist_.begin(), fieldlist_.end(), fldname) != fieldlist_.end();
}

DbResult<void> ProjectScan::close() {
    return s_->close();
}
