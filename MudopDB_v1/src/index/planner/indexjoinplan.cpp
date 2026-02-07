#include "index/planner/indexjoinplan.hpp"
#include "index/query/indexjoinscan.hpp"
#include "record/tablescan.hpp"
#include <stdexcept>

namespace index {

IndexJoinPlan::IndexJoinPlan(std::shared_ptr<Plan> p1,
                             std::shared_ptr<Plan> p2,
                             const metadata::IndexInfo& ii,
                             const std::string& joinfield)
    : p1_(p1), p2_(p2), ii_(ii), joinfield_(joinfield) {
    sch_ = std::make_shared<record::Schema>();
    sch_->add_all(*p1_->schema());
    sch_->add_all(*p2_->schema());
}

std::unique_ptr<Scan> IndexJoinPlan::open() {
    auto s = p1_->open();
    auto s2 = p2_->open();
    auto* ts = dynamic_cast<record::TableScan*>(s2.get());
    if (!ts) {
        throw std::runtime_error("IndexJoinPlan requires TableScan on RHS");
    }
    s2.release();
    auto ts_ptr = std::unique_ptr<record::TableScan>(ts);
    auto idx = ii_.open();
    return std::make_unique<IndexJoinScan>(std::move(s), std::move(idx), joinfield_, std::move(ts_ptr));
}

size_t IndexJoinPlan::blocks_accessed() const {
    return p1_->blocks_accessed()
        + (p1_->records_output() * ii_.blocks_accessed())
        + records_output();
}

size_t IndexJoinPlan::records_output() const {
    return p1_->records_output() * ii_.records_output();
}

size_t IndexJoinPlan::distinct_values(const std::string& fldname) const {
    if (p1_->schema()->has_field(fldname)) {
        return p1_->distinct_values(fldname);
    }
    return p2_->distinct_values(fldname);
}

std::shared_ptr<record::Schema> IndexJoinPlan::schema() const {
    return sch_;
}

} // namespace index
