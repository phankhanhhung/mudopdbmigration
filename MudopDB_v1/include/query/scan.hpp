#ifndef SCAN_HPP
#define SCAN_HPP

#include <string>
#include "query/constant.hpp"
#include "common/result.hpp"

/**
 * Abstract base class for all scan types.
 * This interface defines the contract that all concrete scan implementations must follow.
 *
 * Methods return DbResult<T> for explicit error handling, mirroring
 * Rust's Result<T, TransactionError> pattern in NMDB2/src/query/scan.rs.
 *
 * Callers can use .value() to unwrap (throws on error) or check .is_ok() first.
 */
class Scan {
public:
    virtual ~Scan() = default;

    /**
     * Positions the scan before its first record.
     */
    virtual DbResult<void> before_first() = 0;

    /**
     * Moves the scan to the next record.
     * @return ok(true) if there is a next record, ok(false) if done
     */
    virtual DbResult<bool> next() = 0;

    /**
     * Returns the value of the specified integer field in the current record.
     * @param fldname the name of the field
     * @return ok(value) or err(message)
     */
    virtual DbResult<int> get_int(const std::string& fldname) = 0;

    /**
     * Returns the value of the specified string field in the current record.
     * @param fldname the name of the field
     * @return ok(value) or err(message)
     */
    virtual DbResult<std::string> get_string(const std::string& fldname) = 0;

    /**
     * Returns the value of the specified field as a Constant.
     * @param fldname the name of the field
     * @return ok(value) or err(message)
     */
    virtual DbResult<Constant> get_val(const std::string& fldname) = 0;

    /**
     * Returns true if the scan has the specified field.
     * @param fldname the name of the field
     * @return true if the scan has that field
     */
    virtual bool has_field(const std::string& fldname) const = 0;

    /**
     * Closes the scan and its subscans, if any.
     */
    virtual DbResult<void> close() = 0;
};

#endif // SCAN_HPP
