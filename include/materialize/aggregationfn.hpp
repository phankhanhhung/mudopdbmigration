#ifndef AGGREGATIONFN_HPP
#define AGGREGATIONFN_HPP

#include "query/constant.hpp"
#include <string>
#include <optional>

class Scan;

namespace materialize {

/**
 * Abstract interface for aggregate functions.
 *
 * Corresponds to AggregationFnControl trait in Rust.
 */
class AggregationFn {
public:
    virtual ~AggregationFn() = default;
    virtual void process_first(Scan& s) = 0;
    virtual void process_next(Scan& s) = 0;
    virtual std::string field_name() const = 0;
    virtual std::optional<Constant> value() const = 0;
};

} // namespace materialize

#endif // AGGREGATIONFN_HPP
