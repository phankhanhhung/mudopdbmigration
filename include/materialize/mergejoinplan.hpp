#ifndef MERGEJOINPLAN_HPP
#define MERGEJOINPLAN_HPP

#include "plan/plan.hpp"
#include "materialize/sortplan.hpp"
#include <memory>
#include <string>

namespace tx { class Transaction; }

namespace materialize {

class MergeJoinPlan : public Plan {
public:
    MergeJoinPlan(std::shared_ptr<tx::Transaction> tx,
                  std::shared_ptr<Plan> p1, std::shared_ptr<Plan> p2,
                  const std::string& fldname1, const std::string& fldname2);

    std::unique_ptr<Scan> open() override;
    size_t blocks_accessed() const override;
    size_t records_output() const override;
    size_t distinct_values(const std::string& fldname) const override;
    std::shared_ptr<record::Schema> schema() const override;

private:
    std::shared_ptr<SortPlan> p1_;
    std::shared_ptr<SortPlan> p2_;
    std::string fldname1_;
    std::string fldname2_;
    std::shared_ptr<record::Schema> sch_;
};

} // namespace materialize

#endif // MERGEJOINPLAN_HPP
