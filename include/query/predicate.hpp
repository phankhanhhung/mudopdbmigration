/**
 * @file predicate.hpp
 * @brief Conjunction of terms representing a WHERE clause.
 */

#ifndef PREDICATE_HPP
#define PREDICATE_HPP

#include "query/term.hpp"
#include "query/constant.hpp"
#include <optional>
#include <string>
#include <vector>
#include <memory>

namespace record {
class Schema;
}

class Scan;
class Plan;

/**
 * A predicate is a conjunction (AND) of terms.
 *
 * Corresponds to Predicate in Rust (NMDB2/src/query/predicate.rs)
 */
class Predicate {
public:
    Predicate();
    static Predicate with_term(const Term& t);

    void conjoin_with(const Predicate& pred);

    bool is_satisfied(Scan& s) const;

    size_t reduction_factor(const Plan& p) const;

    std::optional<Predicate> select_sub_pred(std::shared_ptr<record::Schema> sch) const;
    std::optional<Predicate> join_sub_pred(std::shared_ptr<record::Schema> sch1,
                                            std::shared_ptr<record::Schema> sch2) const;

    std::optional<Constant> equates_with_constant(const std::string& fldname) const;
    std::optional<std::string> equates_with_field(const std::string& fldname) const;

    std::string to_string() const;

private:
    std::vector<Term> terms_;
};

#endif // PREDICATE_HPP
