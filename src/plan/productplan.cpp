#include "plan/productplan.hpp"
#include "query/productscan.hpp"
#include "record/schema.hpp"

ProductPlan::ProductPlan(std::shared_ptr<Plan> p1, std::shared_ptr<Plan> p2)
    : p1_(p1), p2_(p2) {
    auto sch = std::make_shared<record::Schema>();
    sch->add_all(*p1_->schema());
    sch->add_all(*p2_->schema());
    schema_ = sch;
}

std::unique_ptr<Scan> ProductPlan::open() {
    auto s1 = p1_->open();
    auto s2 = p2_->open();
    return std::make_unique<ProductScan>(std::move(s1), std::move(s2));
}

size_t ProductPlan::blocks_accessed() const {
    return p1_->blocks_accessed() + (p1_->records_output() * p2_->blocks_accessed());
}

size_t ProductPlan::records_output() const {
    return p1_->records_output() * p2_->records_output();
}

size_t ProductPlan::distinct_values(const std::string& fldname) const {
    if (p1_->schema()->has_field(fldname)) {
        return p1_->distinct_values(fldname);
    }
    return p2_->distinct_values(fldname);
}

std::shared_ptr<record::Schema> ProductPlan::schema() const {
    return schema_;
}
