/**
 * Predicate implementation.
 * Conjunction of Terms representing a SQL WHERE clause.
 */

#include "query/predicate.hpp"
#include "record/schema.hpp"

Predicate::Predicate() {}

Predicate Predicate::with_term(const Term& t) {
    Predicate p;
    p.terms_.push_back(t);
    return p;
}

void Predicate::conjoin_with(const Predicate& pred) {
    terms_.insert(terms_.end(), pred.terms_.begin(), pred.terms_.end());
}

bool Predicate::is_satisfied(Scan& s) const {
    for (const auto& t : terms_) {
        if (!t.is_satisfied(s)) {
            return false;
        }
    }
    return true;
}

size_t Predicate::reduction_factor(const Plan& p) const {
    size_t factor = 1;
    for (const auto& t : terms_) {
        factor *= t.reduction_factor(p);
    }
    return factor;
}

std::optional<Predicate> Predicate::select_sub_pred(std::shared_ptr<record::Schema> sch) const {
    Predicate result;
    for (const auto& t : terms_) {
        if (t.applies_to(*sch)) {
            result.terms_.push_back(t);
        }
    }
    if (result.terms_.empty()) {
        return std::nullopt;
    }
    return result;
}

std::optional<Predicate> Predicate::join_sub_pred(std::shared_ptr<record::Schema> sch1,
                                                   std::shared_ptr<record::Schema> sch2) const {
    Predicate result;
    record::Schema newsch;
    newsch.add_all(*sch1);
    newsch.add_all(*sch2);
    for (const auto& t : terms_) {
        if (!t.applies_to(*sch1) && !t.applies_to(*sch2) && t.applies_to(newsch)) {
            result.terms_.push_back(t);
        }
    }
    if (result.terms_.empty()) {
        return std::nullopt;
    }
    return result;
}

std::optional<Constant> Predicate::equates_with_constant(const std::string& fldname) const {
    for (const auto& t : terms_) {
        auto c = t.equates_with_constant(fldname);
        if (c.has_value()) {
            return c;
        }
    }
    return std::nullopt;
}

std::optional<std::string> Predicate::equates_with_field(const std::string& fldname) const {
    for (const auto& t : terms_) {
        auto s = t.equates_with_field(fldname);
        if (s.has_value()) {
            return s;
        }
    }
    return std::nullopt;
}

std::string Predicate::to_string() const {
    if (terms_.empty()) return "";
    std::string result = terms_[0].to_string();
    for (size_t i = 1; i < terms_.size(); i++) {
        result += " and " + terms_[i].to_string();
    }
    return result;
}
