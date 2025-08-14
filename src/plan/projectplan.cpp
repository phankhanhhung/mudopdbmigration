#include "plan/projectplan.hpp"
#include "query/projectscan.hpp"
#include "record/schema.hpp"

ProjectPlan::ProjectPlan(std::shared_ptr<Plan> p, const std::vector<std::string>& field_list)
    : p_(p) {
    auto sch = std::make_shared<record::Schema>();
    for (const auto& fldname : field_list) {
        sch->add(fldname, *p_->schema());
    }
    schema_ = sch;
}

std::unique_ptr<Scan> ProjectPlan::open() {
    auto s = p_->open();
    return std::make_unique<ProjectScan>(std::move(s), schema_->fields());
}

size_t ProjectPlan::blocks_accessed() const {
    return p_->blocks_accessed();
}

size_t ProjectPlan::records_output() const {
    return p_->records_output();
}

size_t ProjectPlan::distinct_values(const std::string& fldname) const {
    return p_->distinct_values(fldname);
}

std::shared_ptr<record::Schema> ProjectPlan::schema() const {
    return schema_;
}
