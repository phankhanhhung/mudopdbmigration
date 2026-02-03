#ifndef LAYOUT_HPP
#define LAYOUT_HPP

#include "record/schema.hpp"
#include "file/page.hpp"
#include <memory>
#include <unordered_map>

namespace record {

/**
 * Layout describes the physical layout of a record.
 *
 * Calculates:
 * - Offset of each field within a record slot
 * - Total slot size (4-byte flag + all fields)
 *
 * Corresponds to Layout in Rust (NMDB2/src/record/layout.rs)
 */
class Layout {
public:
    /**
     * Creates a layout from a schema.
     * Automatically calculates field offsets.
     *
     * @param schema the table schema
     */
    explicit Layout(std::shared_ptr<Schema> schema);

    /**
     * Creates a layout with explicit metadata.
     * (Used for deserialization from metadata catalog)
     *
     * @param schema the table schema
     * @param offsets field name to offset map
     * @param slotsize the total slot size
     */
    Layout(std::shared_ptr<Schema> schema,
           std::unordered_map<std::string, size_t> offsets,
           size_t slotsize);

    /**
     * Returns the schema.
     *
     * @return shared pointer to schema
     */
    std::shared_ptr<Schema> schema() const;

    /**
     * Returns the offset of a field within a slot.
     *
     * @param fldname the field name
     * @return the byte offset
     */
    size_t offset(const std::string& fldname) const;

    /**
     * Returns the total size of a record slot.
     *
     * @return slot size in bytes
     */
    size_t slot_size() const;

private:
    /**
     * Calculates the storage size of a field.
     *
     * @param fldname the field name
     * @return bytes needed to store the field
     */
    size_t length_in_bytes(const std::string& fldname) const;

private:
    std::shared_ptr<Schema> schema_;
    std::unordered_map<std::string, size_t> offsets_;
    size_t slotsize_;
};

} // namespace record

#endif // LAYOUT_HPP
