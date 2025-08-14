#include "record/schema.hpp"

namespace record {

Schema::Schema() {}

void Schema::add_field(const std::string& fldname, Type type, size_t length) {
    fields_.push_back(fldname);
    info_[fldname] = FieldInfo{type, length};
}

void Schema::add_int_field(const std::string& fldname) {
    add_field(fldname, Type::INTEGER, 0);
}

void Schema::add_string_field(const std::string& fldname, size_t length) {
    add_field(fldname, Type::VARCHAR, length);
}

void Schema::add(const std::string& fldname, const Schema& sch) {
    Type fldtype = sch.type(fldname);
    size_t fldlen = sch.length(fldname);
    add_field(fldname, fldtype, fldlen);
}

void Schema::add_all(const Schema& sch) {
    for (const auto& fldname : sch.fields()) {
        add(fldname, sch);
    }
}

const std::vector<std::string>& Schema::fields() const {
    return fields_;
}

bool Schema::has_field(const std::string& fldname) const {
    return info_.find(fldname) != info_.end();
}

Type Schema::type(const std::string& fldname) const {
    return info_.at(fldname).type;
}

size_t Schema::length(const std::string& fldname) const {
    return info_.at(fldname).length;
}

} // namespace record
