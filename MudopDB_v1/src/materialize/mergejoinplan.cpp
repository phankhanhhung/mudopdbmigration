#include "materialize/mergejoinplan.hpp"
#include "materialize/mergejoinscan.hpp"
#include "materialize/sortscan.hpp"
#include "record/schema.hpp"
#include <algorithm>

namespace materialize {

MergeJoinPlan::MergeJoinPlan(std::shared_ptr<tx::Transaction> tx,
                              std::shared_ptr<Plan> p1, std::shared_ptr<Plan> p2,
                              const std::string& fldname1, const std::string& fldname2)
    : fldname1_(fldname1), fldname2_(fldname2) {
    std::vector<std::string> sortlist1 = {fldname1};
    p1_ = std::make_shared<SortPlan>(tx, p1, sortlist1);

    std::vector<std::string> sortlist2 = {fldname2};
    p2_ = std::make_shared<SortPlan>(tx, p2, sortlist2);

    auto sch = std::make_shared<record::Schema>();
    sch->add_all(*p1_->schema());
    sch->add_all(*p2_->schema());
    sch_ = sch;
}

std::unique_ptr<Scan> MergeJoinPlan::open() {
    auto s1 = p1_->open();
    auto s2_raw = p2_->open();
    // SortPlan::open() returns a SortScan
    auto s2 = std::unique_ptr<SortScan>(
        dynamic_cast<SortScan*>(s2_raw.release()));
    return std::make_unique<MergeJoinScan>(std::move(s1), std::move(s2),
                                            fldname1_, fldname2_);
}

size_t MergeJoinPlan::blocks_accessed() const {
    return p1_->blocks_accessed() + p2_->blocks_accessed();
}

size_t MergeJoinPlan::records_output() const {
    size_t maxvals = std::max(p1_->distinct_values(fldname1_),
                              p2_->distinct_values(fldname2_));
    if (maxvals == 0) maxvals = 1;
    return (p1_->records_output() * p2_->records_output()) / maxvals;
}

size_t MergeJoinPlan::distinct_values(const std::string& fldname) const {
    if (p1_->schema()->has_field(fldname)) {
        return p1_->distinct_values(fldname);
    }
    return p2_->distinct_values(fldname);
}

std::shared_ptr<record::Schema> MergeJoinPlan::schema() const {
    return sch_;
}

} // namespace materialize
