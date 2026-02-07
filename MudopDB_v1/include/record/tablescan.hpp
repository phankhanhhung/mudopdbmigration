#ifndef TABLESCAN_HPP
#define TABLESCAN_HPP

#include "record/recordpage.hpp"
#include "record/layout.hpp"
#include "record/rid.hpp"
#include "query/scan.hpp"
#include "query/constant.hpp"
#include <memory>
#include <optional>
#include <string>

namespace tx {
class Transaction;
}

namespace record {

/**
 * TableScan provides sequential access to table records.
 *
 * Implements the Scan interface for reading records.
 * Provides additional methods for updates (insert, delete, set).
 *
 * Uses Transaction layer for all data access.
 *
 * Corresponds to TableScan in Rust (NMDB2/src/record/tablescan.rs)
 */
class TableScan : public Scan {
public:
    /**
     * Creates a table scan.
     *
     * @param tx the transaction
     * @param tablename the table name
     * @param layout the table layout
     */
    TableScan(std::shared_ptr<tx::Transaction> tx,
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

    // Update operations
    void set_val(const std::string& fldname, const Constant& val);
    void set_int(const std::string& fldname, int32_t val);
    void set_string(const std::string& fldname, const std::string& val);
    void insert();
    void delete_record();

    std::optional<RID> get_rid() const;
    void move_to_rid(const RID& rid);

private:
    void move_to_block(int32_t blknum);
    void move_to_new_block();
    bool at_last_block() const;

    std::shared_ptr<tx::Transaction> tx_;
    Layout layout_;
    std::unique_ptr<RecordPage> rp_;
    std::string filename_;
    std::optional<size_t> currentslot_;
};

} // namespace record

#endif // TABLESCAN_HPP
