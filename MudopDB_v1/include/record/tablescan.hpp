#ifndef TABLESCAN_HPP
#define TABLESCAN_HPP

#include "record/recordpage.hpp"
#include "record/layout.hpp"
#include "record/rid.hpp"
#include "query/updatescan.hpp"
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
 * Implements the UpdateScan interface for reading and writing records.
 * Uses Transaction layer for all data access.
 *
 * Corresponds to TableScan in Rust (NMDB2/src/record/tablescan.rs)
 */
class TableScan : public UpdateScan {
public:
    TableScan(std::shared_ptr<tx::Transaction> tx,
              const std::string& tablename,
              const Layout& layout);

    // Scan interface implementation
    DbResult<void> before_first() override;
    DbResult<bool> next() override;
    DbResult<int> get_int(const std::string& fldname) override;
    DbResult<std::string> get_string(const std::string& fldname) override;
    DbResult<Constant> get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    DbResult<void> close() override;

    // UpdateScan interface implementation
    DbResult<void> set_val(const std::string& fldname, const Constant& val) override;
    DbResult<void> set_int(const std::string& fldname, int32_t val) override;
    DbResult<void> set_string(const std::string& fldname, const std::string& val) override;
    DbResult<void> insert() override;
    DbResult<void> delete_record() override;
    std::optional<RID> get_rid() const override;
    DbResult<void> move_to_rid(const RID& rid) override;

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
