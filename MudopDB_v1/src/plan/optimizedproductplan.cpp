#include "plan/optimizedproductplan.hpp"
#include "plan/productplan.hpp"
#include "query/scan.hpp"

OptimizedProductPlan::OptimizedProductPlan(
    std::shared_ptr<Plan> p1, std::shared_ptr<Plan> p2) {
    auto prod1 = std::make_shared<ProductPlan>(p1, p2);
    auto prod2 = std::make_shared<ProductPlan>(p2, p1);
    if (prod1->blocks_accessed() < prod2->blocks_accessed()) {
        bestplan_ = prod1;
    } else {
        bestplan_ = prod2;
    }
}

std::unique_ptr<Scan> OptimizedProductPlan::open() {
    return bestplan_->open();
}

size_t OptimizedProductPlan::blocks_accessed() const {
    return bestplan_->blocks_accessed();
}

size_t OptimizedProductPlan::records_output() const {
    return bestplan_->records_output();
}

size_t OptimizedProductPlan::distinct_values(const std::string& fldname) const {
    return bestplan_->distinct_values(fldname);
}

std::shared_ptr<record::Schema> OptimizedProductPlan::schema() const {
    return bestplan_->schema();
}
