#ifndef SCAN_HPP
#define SCAN_HPP

#include <string>
#include "query/constant.hpp"

// Forward declarations for error types

class Scan {
public:
    virtual ~Scan() = default;

    virtual void before_first() = 0;

    virtual bool next() = 0;

    virtual int get_int(const std::string& fldname) = 0;

    virtual std::string get_string(const std::string& fldname) = 0;

    virtual Constant get_val(const std::string& fldname) = 0;

    virtual bool has_field(const std::string& fldname) const = 0;

    virtual void close() = 0;
};

#endif // SCAN_HPP