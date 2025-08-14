#ifndef SELECTPLAN_HPP
#define SELECTPLAN_HPP

#include "plan/plan.hpp"
#include "query/predicate.hpp"
#include <memory>

class SelectPlan : public Plan {
public:
    SelectPlan(std::shared_ptr<Plan> p, const Predicate& pred);

    std::unique_ptr<Scan> open() override;
    size_t blocks_accessed() const override;
    size_t records_output() const override;
    size_t distinct_values(const std::string& fldname) const override;
    std::shared_ptr<record::Schema> schema() const override;

private:
    std::shared_ptr<Plan> p_;
    Predicate pred_;
};

#endif // SELECTPLAN_HPP
