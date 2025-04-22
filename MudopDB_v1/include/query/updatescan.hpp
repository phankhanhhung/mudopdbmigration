#ifndef UPDATESCAN_HPP
#define UPDATESCAN_HPP

#include "query/scan.hpp"
#include "query/constant.hpp"
#include "record/rid.hpp"
#include <optional>
#include <string>

/**
 * UpdateScan extends Scan with write operations.
 *
 * Corresponds to UpdateScanControl trait in Rust (NMDB2/src/query/updatescan.rs)
 */
class UpdateScan : public Scan {
public:
    virtual ~UpdateScan() = default;

    virtual void set_val(const std::string& fldname, const Constant& val) = 0;
    virtual void set_int(const std::string& fldname, int32_t val) = 0;
    virtual void set_string(const std::string& fldname, const std::string& val) = 0;
    virtual void insert() = 0;
    virtual void delete_record() = 0;
    virtual std::optional<record::RID> get_rid() const = 0;
    virtual void move_to_rid(const record::RID& rid) = 0;
};

#endif // UPDATESCAN_HPP
