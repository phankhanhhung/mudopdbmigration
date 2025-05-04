#ifndef SCAN_HPP
#define SCAN_HPP

#include <string>
#include "query/constant.hpp"

// Forward declarations for error types
class TransactionError;
class AbortError;

/**
 * Abstract base class for all scan types.
 * This interface defines the contract that all concrete scan implementations must follow.
 *
 * Corresponds to the ScanControl trait in Rust (NMDB2/src/query/scan.rs)
 */
class Scan {
public:
    virtual ~Scan() = default;

    /**
     * Positions the scan before its first record.
     * A subsequent call to next() will return the first record.
     */
    virtual void before_first() = 0;

    /**
     * Moves the scan to the next record.
     * @return true if there is a next record, false otherwise
     */
    virtual bool next() = 0;

    /**
     * Returns the value of the specified integer field in the current record.
     * @param fldname the name of the field
     * @return the field's integer value
     */
    virtual int get_int(const std::string& fldname) = 0;

    /**
     * Returns the value of the specified string field in the current record.
     * @param fldname the name of the field
     * @return the field's string value
     */
    virtual std::string get_string(const std::string& fldname) = 0;

    /**
     * Returns the value of the specified field in the current record,
     * expressed as a Constant.
     * @param fldname the name of the field
     * @return the field's value as a Constant
     */
    virtual Constant get_val(const std::string& fldname) = 0;

    /**
     * Returns true if the scan has the specified field.
     * @param fldname the name of the field
     * @return true if the scan has that field
     */
    virtual bool has_field(const std::string& fldname) const = 0;

    /**
     * Closes the scan and its subscans, if any.
     */
    virtual void close() = 0;
};

#endif // SCAN_HPP