#ifndef OPTIMIZEDPRODUCTPLAN_HPP
#define OPTIMIZEDPRODUCTPLAN_HPP

#include "plan/plan.hpp"
#include <memory>

/**
 * OptimizedProductPlan wraps two plans and internally selects
 * the product order that minimizes block accesses.
 *
 * Corresponds to OptimizedProductPlan in Rust (NMDB2/src/plan/optimizedproductplan.rs)
 */
class OptimizedProductPlan : public Plan {
public:
    OptimizedProductPlan(std::shared_ptr<Plan> p1, std::shared_ptr<Plan> p2);

    std::unique_ptr<Scan> open() override;
    size_t blocks_accessed() const override;
    size_t records_output() const override;
    size_t distinct_values(const std::string& fldname) const override;
    std::shared_ptr<record::Schema> schema() const override;

private:
    std::shared_ptr<Plan> bestplan_;
};

#endif // OPTIMIZEDPRODUCTPLAN_HPP
