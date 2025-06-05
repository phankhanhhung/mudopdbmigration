#ifndef PROJECTPLAN_HPP
#define PROJECTPLAN_HPP

#include "plan/plan.hpp"
#include <memory>
#include <vector>
#include <string>

class ProjectPlan : public Plan {
public:
    ProjectPlan(std::shared_ptr<Plan> p, const std::vector<std::string>& field_list);

    std::unique_ptr<Scan> open() override;
    size_t blocks_accessed() const override;
    size_t records_output() const override;
    size_t distinct_values(const std::string& fldname) const override;
    std::shared_ptr<record::Schema> schema() const override;

private:
    std::shared_ptr<Plan> p_;
    std::shared_ptr<record::Schema> schema_;
};

#endif // PROJECTPLAN_HPP
