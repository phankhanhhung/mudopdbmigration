#ifndef GROUPBYPLAN_HPP
#define GROUPBYPLAN_HPP

#include "plan/plan.hpp"
#include "materialize/aggregationfn.hpp"
#include "materialize/sortplan.hpp"
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace tx { class Transaction; }

namespace materialize {

class GroupByPlan : public Plan {
public:
    using AggFnFactory = std::function<std::unique_ptr<AggregationFn>()>;

    GroupByPlan(std::shared_ptr<tx::Transaction> tx,
                std::shared_ptr<Plan> p,
                const std::vector<std::string>& groupfields,
                const std::vector<AggFnFactory>& aggfn_factories);

    std::unique_ptr<Scan> open() override;
    size_t blocks_accessed() const override;
    size_t records_output() const override;
    size_t distinct_values(const std::string& fldname) const override;
    std::shared_ptr<record::Schema> schema() const override;

private:
    std::shared_ptr<SortPlan> p_;
    std::vector<std::string> groupfields_;
    std::vector<AggFnFactory> aggfn_factories_;
    std::shared_ptr<record::Schema> sch_;
};

} // namespace materialize

#endif // GROUPBYPLAN_HPP
