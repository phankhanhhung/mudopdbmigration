/**
 * SelectPlan implementation.
 * Plan node that applies a selection predicate to filter records.
 */

#include "plan/selectplan.hpp"
#include "query/selectscan.hpp"
#include "record/schema.hpp"
#include <algorithm>

SelectPlan::SelectPlan(std::shared_ptr<Plan> p, const Predicate& pred)
    : p_(p), pred_(pred) {}

std::unique_ptr<Scan> SelectPlan::open() {
    auto s = p_->open();
    return std::make_unique<SelectScan>(std::move(s), pred_);
}

size_t SelectPlan::blocks_accessed() const {
    return p_->blocks_accessed();
}

size_t SelectPlan::records_output() const {
    auto rf = pred_.reduction_factor(*p_);
    if (rf == 0) return p_->records_output();
    return p_->records_output() / rf;
}

size_t SelectPlan::distinct_values(const std::string& fldname) const {
    if (pred_.equates_with_constant(fldname).has_value()) {
        return 1;
    }
    auto fldname2 = pred_.equates_with_field(fldname);
    if (fldname2.has_value()) {
        return std::min(p_->distinct_values(fldname),
                        p_->distinct_values(fldname2.value()));
    }
    return p_->distinct_values(fldname);
}

std::shared_ptr<record::Schema> SelectPlan::schema() const {
    return p_->schema();
}
