#include "query/term.hpp"
#include "query/scan.hpp"
#include "plan/plan.hpp"
#include "record/schema.hpp"
#include <algorithm>

Term::Term(const Expression& lhs, const Expression& rhs)
    : lhs_(lhs), rhs_(rhs) {}

bool Term::is_satisfied(Scan& s) const {
    Constant lhsval = lhs_.evaluate(s);
    Constant rhsval = rhs_.evaluate(s);
    return lhsval == rhsval;
}

size_t Term::reduction_factor(const Plan& p) const {
    auto lhs_name = lhs_.as_field_name();
    auto rhs_name = rhs_.as_field_name();
    if (lhs_name.has_value() && rhs_name.has_value()) {
        return std::max(p.distinct_values(lhs_name.value()),
                        p.distinct_values(rhs_name.value()));
    }
    if (lhs_name.has_value()) {
        return p.distinct_values(lhs_name.value());
    }
    if (rhs_name.has_value()) {
        return p.distinct_values(rhs_name.value());
    }
    if (lhs_.as_constant() == rhs_.as_constant()) {
        return 1;
    }
    return SIZE_MAX;
}

std::optional<Constant> Term::equates_with_constant(const std::string& fldname) const {
    if (auto lhs_name = lhs_.as_field_name()) {
        if (*lhs_name != fldname) return std::nullopt;
        if (auto rhs_const = rhs_.as_constant()) {
            return rhs_const;
        }
    }
    if (auto rhs_name = rhs_.as_field_name()) {
        if (*rhs_name != fldname) return std::nullopt;
        if (auto lhs_const = lhs_.as_constant()) {
            return lhs_const;
        }
    }
    return std::nullopt;
}

std::optional<std::string> Term::equates_with_field(const std::string& fldname) const {
    auto lhs_name = lhs_.as_field_name();
    auto rhs_name = rhs_.as_field_name();
    if (lhs_name.has_value() && rhs_name.has_value()) {
        if (*lhs_name == fldname) return rhs_name;
        if (*rhs_name == fldname) return lhs_name;
    }
    return std::nullopt;
}

bool Term::applies_to(const record::Schema& sch) const {
    return lhs_.applies_to(sch) && rhs_.applies_to(sch);
}

std::string Term::to_string() const {
    return lhs_.to_string() + "=" + rhs_.to_string();
}
