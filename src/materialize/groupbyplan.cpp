/**
 * GroupByPlan implementation.
 * Plan node for GROUP BY with aggregation, built on top of SortPlan.
 */

#include "materialize/groupbyplan.hpp"
#include "materialize/groupbyscan.hpp"
#include "record/schema.hpp"
#include "tx/transaction.hpp"

namespace materialize {

GroupByPlan::GroupByPlan(std::shared_ptr<tx::Transaction> tx,
                         std::shared_ptr<Plan> p,
                         const std::vector<std::string>& groupfields,
                         const std::vector<AggFnFactory>& aggfn_factories)
    : groupfields_(groupfields), aggfn_factories_(aggfn_factories) {
    p_ = std::make_shared<SortPlan>(tx, p, groupfields);
    auto sch = std::make_shared<record::Schema>();
    for (const auto& fldname : groupfields) {
        sch->add(fldname, *p_->schema());
    }
    for (const auto& factory : aggfn_factories_) {
        auto fn = factory();
        sch->add_int_field(fn->field_name());
    }
    sch_ = sch;
}

std::unique_ptr<Scan> GroupByPlan::open() {
    auto s = p_->open();
    std::vector<std::unique_ptr<AggregationFn>> aggfns;
    for (const auto& factory : aggfn_factories_) {
        aggfns.push_back(factory());
    }
    return std::make_unique<GroupByScan>(std::move(s), groupfields_, std::move(aggfns));
}

size_t GroupByPlan::blocks_accessed() const {
    return p_->blocks_accessed();
}

size_t GroupByPlan::records_output() const {
    size_t numgroups = 1;
    for (const auto& fldname : groupfields_) {
        numgroups *= p_->distinct_values(fldname);
    }
    return numgroups;
}

size_t GroupByPlan::distinct_values(const std::string& fldname) const {
    if (p_->schema()->has_field(fldname)) {
        return p_->distinct_values(fldname);
    }
    return records_output();
}

std::shared_ptr<record::Schema> GroupByPlan::schema() const {
    return sch_;
}

} // namespace materialize
