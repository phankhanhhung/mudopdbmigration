#ifndef PRODUCTPLAN_HPP
#define PRODUCTPLAN_HPP

#include "plan/plan.hpp"
#include <memory>

class ProductPlan : public Plan {
public:
    ProductPlan(std::shared_ptr<Plan> p1, std::shared_ptr<Plan> p2);

    std::unique_ptr<Scan> open() override;
    size_t blocks_accessed() const override;
    size_t records_output() const override;
    size_t distinct_values(const std::string& fldname) const override;
    std::shared_ptr<record::Schema> schema() const override;

private:
    std::shared_ptr<Plan> p1_;
    std::shared_ptr<Plan> p2_;
    std::shared_ptr<record::Schema> schema_;
};

#endif // PRODUCTPLAN_HPP
