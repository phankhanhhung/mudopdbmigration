#ifndef SELECTSCAN_HPP
#define SELECTSCAN_HPP

#include "query/updatescan.hpp"
#include "query/predicate.hpp"
#include <memory>

/**
 * SelectScan filters records using a predicate.
 * Supports update operations by delegating to the inner scan.
 *
 * Corresponds to SelectScan in Rust (NMDB2/src/query/selectscan.rs)
 */
class SelectScan : public UpdateScan {
public:
    SelectScan(std::unique_ptr<Scan> s, const Predicate& pred);

    DbResult<void> before_first() override;
    DbResult<bool> next() override;
    DbResult<int> get_int(const std::string& fldname) override;
    DbResult<std::string> get_string(const std::string& fldname) override;
    DbResult<Constant> get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    DbResult<void> close() override;

    // UpdateScan delegation
    DbResult<void> set_val(const std::string& fldname, const Constant& val) override;
    DbResult<void> set_int(const std::string& fldname, int32_t val) override;
    DbResult<void> set_string(const std::string& fldname, const std::string& val) override;
    DbResult<void> insert() override;
    DbResult<void> delete_record() override;
    std::optional<record::RID> get_rid() const override;
    DbResult<void> move_to_rid(const record::RID& rid) override;

private:
    UpdateScan* get_update_scan() const;

    std::unique_ptr<Scan> s_;
    Predicate pred_;
};

#endif // SELECTSCAN_HPP
