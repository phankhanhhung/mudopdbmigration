#ifndef RECORDPAGE_HPP
#define RECORDPAGE_HPP

#include "record/layout.hpp"
#include "buffer/buffer.hpp"
#include "file/blockid.hpp"
#include <memory>
#include <optional>
#include <cstdint>

namespace record {

/**
 * RecordPage manages records within a single page.
 *
 * Page Format:
 * [Slot 0: flag + fields][Slot 1: flag + fields][...]
 *
 * Flag: 0 = EMPTY, 1 = USED
 *
 * NOTE: Phase 4 version does NOT use Transaction layer (Phase 5).
 * Instead, directly uses Buffer for simplicity.
 *
 * Corresponds to RecordPage in Rust (NMDB2/src/record/recordpage.rs)
 */
class RecordPage {
public:
    /**
     * Creates a record page for a block.
     *
     * @param buff the buffer containing the page
     * @param layout the record layout
     */
    RecordPage(buffer::Buffer& buff, const Layout& layout);

    /**
     * Gets an integer field value.
     *
     * @param slot the slot number
     * @param fldname the field name
     * @return the integer value
     */
    int32_t get_int(size_t slot, const std::string& fldname);

    /**
     * Gets a string field value.
     *
     * @param slot the slot number
     * @param fldname the field name
     * @return the string value
     */
    std::string get_string(size_t slot, const std::string& fldname);

    /**
     * Sets an integer field value.
     *
     * @param slot the slot number
     * @param fldname the field name
     * @param val the integer value
     */
    void set_int(size_t slot, const std::string& fldname, int32_t val);

    /**
     * Sets a string field value.
     *
     * @param slot the slot number
     * @param fldname the field name
     * @param val the string value
     */
    void set_string(size_t slot, const std::string& fldname, const std::string& val);

    /**
     * Deletes a record (sets flag to EMPTY).
     *
     * @param slot the slot number
     */
    void delete_record(size_t slot);

    /**
     * Formats the page (sets all slots to EMPTY).
     */
    void format();

    /**
     * Finds the next used slot after the given slot.
     *
     * @param slot starting slot (or std::nullopt for start of page)
     * @return next used slot, or std::nullopt if none
     */
    std::optional<size_t> next_after(std::optional<size_t> slot);

    /**
     * Finds the next empty slot and marks it as USED.
     *
     * @param slot starting slot (or std::nullopt for start of page)
     * @return newly allocated slot, or std::nullopt if page full
     */
    std::optional<size_t> insert_after(std::optional<size_t> slot);

    /**
     * Returns the block ID of this page.
     *
     * @return block ID
     */
    const file::BlockId& block() const;

private:
    enum class Flag : int32_t {
        EMPTY = 0,
        USED = 1
    };

    /**
     * Sets the flag for a slot.
     */
    void set_flag(size_t slot, Flag flag);

    /**
     * Gets the flag for a slot.
     */
    Flag get_flag(size_t slot);

    /**
     * Searches for a slot with the given flag.
     */
    std::optional<size_t> search_after(std::optional<size_t> slot, Flag flag);

    /**
     * Checks if a slot number is valid (fits in page).
     */
    bool is_valid_slot(size_t slot) const;

    /**
     * Calculates the byte offset of a slot.
     */
    size_t offset(size_t slot) const;

private:
    buffer::Buffer& buff_;
    Layout layout_;
};

} // namespace record

#endif // RECORDPAGE_HPP
