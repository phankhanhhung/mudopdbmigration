#ifndef SCHEMA_HPP
#define SCHEMA_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace record {

/**
 * Type enumeration for field types.
 */
enum class Type : int32_t {
    INTEGER = 4,   // 4 bytes
    VARCHAR = 12   // Variable length (max specified in schema)
};

/**
 * Schema defines the structure of a table.
 *
 * A schema consists of:
 * - Ordered list of field names
 * - Type and length for each field
 *
 * Corresponds to Schema in Rust (NMDB2/src/record/schema.rs)
 */
class Schema {
public:
    Schema();

    /**
     * Adds a field to the schema.
     *
     * @param fldname the field name
     * @param type the field type
     * @param length the field length (for VARCHAR, 0 for INTEGER)
     */
    void add_field(const std::string& fldname, Type type, size_t length);

    /**
     * Adds an integer field.
     *
     * @param fldname the field name
     */
    void add_int_field(const std::string& fldname);

    /**
     * Adds a string field.
     *
     * @param fldname the field name
     * @param length the maximum string length
     */
    void add_string_field(const std::string& fldname, size_t length);

    /**
     * Adds a field from another schema.
     *
     * @param fldname the field name
     * @param sch the source schema
     */
    void add(const std::string& fldname, const Schema& sch);

    /**
     * Adds all fields from another schema.
     *
     * @param sch the source schema
     */
    void add_all(const Schema& sch);

    /**
     * Returns the ordered list of field names.
     *
     * @return vector of field names
     */
    const std::vector<std::string>& fields() const;

    /**
     * Checks if schema has a field.
     *
     * @param fldname the field name
     * @return true if field exists
     */
    bool has_field(const std::string& fldname) const;

    /**
     * Returns the type of a field.
     *
     * @param fldname the field name
     * @return the field type
     */
    Type type(const std::string& fldname) const;

    /**
     * Returns the length of a field.
     *
     * @param fldname the field name
     * @return the field length (for VARCHAR, 0 for INTEGER)
     */
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
