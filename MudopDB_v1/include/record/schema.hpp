#ifndef SCHEMA_HPP
#define SCHEMA_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace record {

enum class Type : int32_t {
    INTEGER = 4,   // 4 bytes
    VARCHAR = 12   // Variable length (max specified in schema)
};

class Schema {
public:
    Schema();

    void add_field(const std::string& fldname, Type type, size_t length);

    void add_int_field(const std::string& fldname);

    void add_string_field(const std::string& fldname, size_t length);

    void add(const std::string& fldname, const Schema& sch);

    void add_all(const Schema& sch);

    const std::vector<std::string>& fields() const;

    bool has_field(const std::string& fldname) const;

    Type type(const std::string& fldname) const;

    size_t length(const std::string& fldname) const;

private:
    struct FieldInfo {
        Type type;
        size_t length;
    };

    std::vector<std::string> fields_;
    std::unordered_map<std::string, FieldInfo> info_;
};

} // namespace record

#endif // SCHEMA_HPP
