#ifndef HEURISTICQUERYPLANNER_HPP
#define HEURISTICQUERYPLANNER_HPP

#include "plan/queryplanner.hpp"
#include "opt/tableplanner.hpp"
#include <memory>
#include <vector>

namespace metadata { class MetadataMgr; }

namespace opt {

class HeuristicQueryPlanner : public QueryPlanner {
public:
    explicit HeuristicQueryPlanner(std::shared_ptr<metadata::MetadataMgr> mdm);

    std::shared_ptr<Plan> create_plan(
        const parse::QueryData& data,
        std::shared_ptr<tx::Transaction> tx) override;

private:
    std::shared_ptr<Plan> get_lowest_select_plan();
    std::shared_ptr<Plan> get_lowest_join_plan(const std::shared_ptr<Plan>& current);
    std::shared_ptr<Plan> get_lowest_product_plan(const std::shared_ptr<Plan>& current);

    std::vector<TablePlanner> tableplanners_;
    std::shared_ptr<metadata::MetadataMgr> mdm_;
};

} // namespace opt

#endif // HEURISTICQUERYPLANNER_HPP
