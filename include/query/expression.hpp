/**
 * @file expression.hpp
 * @brief SQL expression: either a field name or a constant value.
 */

#ifndef EXPRESSION_HPP
#define EXPRESSION_HPP

#include "query/constant.hpp"
#include <optional>
#include <string>

namespace record {
class Schema;
}

class Scan;

/**
 * An expression is either a constant or a field name.
 *
 * Corresponds to Expression in Rust (NMDB2/src/query/expression.rs)
 */
class Expression {
public:
    static Expression with_constant(const Constant& val);
    static Expression with_string(const std::string& fldname);

    Constant evaluate(Scan& s) const;

    std::optional<Constant> as_constant() const;
    std::optional<std::string> as_field_name() const;

    bool applies_to(const record::Schema& sch) const;

    std::string to_string() const;

private:
    std::optional<Constant> val_;
    std::optional<std::string> fldname_;
};

#endif // EXPRESSION_HPP
