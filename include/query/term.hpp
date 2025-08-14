#ifndef TERM_HPP
#define TERM_HPP

#include "query/expression.hpp"
#include "query/constant.hpp"
#include <optional>
#include <string>

namespace record {
class Schema;
}

class Scan;
class Plan;

/**
 * A term is a comparison between two expressions (lhs = rhs).
 *
 * Corresponds to Term in Rust (NMDB2/src/query/term.rs)
 */
class Term {
public:
    Term(const Expression& lhs, const Expression& rhs);

    bool is_satisfied(Scan& s) const;

    size_t reduction_factor(const Plan& p) const;

    std::optional<Constant> equates_with_constant(const std::string& fldname) const;
    std::optional<std::string> equates_with_field(const std::string& fldname) const;

    bool applies_to(const record::Schema& sch) const;

    std::string to_string() const;

private:
    Expression lhs_;
    Expression rhs_;
};

#endif // TERM_HPP
