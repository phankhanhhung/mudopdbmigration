#ifndef TABLESCAN_HPP
#define TABLESCAN_HPP

#include "record/recordpage.hpp"
#include "record/layout.hpp"
#include "record/rid.hpp"
#include "buffer/buffermgr.hpp"
#include "query/scan.hpp"
#include "query/constant.hpp"
#include <memory>
#include <optional>
#include <string>

namespace record {

/**
 * TableScan provides sequential access to table records.
 *
 * Implements the Scan interface for reading records.
 * Provides additional methods for updates (insert, delete, set).
 *
 * NOTE: Phase 4 version simplified - no Transaction layer yet.
 * Directly uses BufferMgr.
 *
 * Corresponds to TableScan in Rust (NMDB2/src/record/tablescan.rs)
 */
class TableScan : public Scan {
public:
    /**
     * Creates a table scan.
     *
     * @param bm the buffer manager
     * @param tablename the table name
     * @param layout the table layout
     */
    TableScan(std::shared_ptr<buffer::BufferMgr> bm,
              const std::string& tablename,
              const Layout& layout);

    // Scan interface implementation
    void before_first() override;
    bool next() override;
    int get_int(const std::string& fldname) override;
    std::string get_string(const std::string& fldname) override;
    Constant get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    void close() override;

    // Update operations (not in Scan interface)

    /**
     * Sets a field value.
     */
    void set_val(const std::string& fldname, const Constant& val);

    /**
     * Sets an integer field.
     */
    void set_int(const std::string& fldname, int32_t val);

    /**
     * Sets a string field.
     */
    void set_string(const std::string& fldname, const std::string& val);

    /**
     * Inserts a new record.
     * Finds an empty slot, possibly creating a new block.
     */
    void insert();

    /**
     * Deletes the current record.
     */
    void delete_record();

    /**
     * Returns the RID of the current record.
     *
     * @return the RID, or std::nullopt if before first or after last
     */
    std::optional<RID> get_rid() const;

    /**
     * Moves to a specific record.
     *
     * @param rid the record ID
     */
    void move_to_rid(const RID& rid);

private:
    /**
     * Moves to a specific block.
     */
    void move_to_block(int32_t blknum);

    /**
     * Creates and moves to a new block.
     */
    void move_to_new_block();

    /**
     * Checks if at the last block.
     */
    bool at_last_block() const;

private:
    std::shared_ptr<buffer::BufferMgr> bm_;
    Layout layout_;
    std::unique_ptr<RecordPage> rp_;
    std::string filename_;
    std::optional<size_t> currentslot_;
    std::optional<size_t> current_buffer_idx_;
};

} // namespace record

#endif // TABLESCAN_HPP
