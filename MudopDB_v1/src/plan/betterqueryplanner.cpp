#include "plan/betterqueryplanner.hpp"
#include "plan/tableplan.hpp"
#include "plan/selectplan.hpp"
#include "plan/projectplan.hpp"
#include "plan/productplan.hpp"
#include "parse/querydata.hpp"
#include "parse/parser.hpp"
#include "metadata/metadatamgr.hpp"
#include "tx/transaction.hpp"

BetterQueryPlanner::BetterQueryPlanner(std::shared_ptr<metadata::MetadataMgr> mdm)
    : mdm_(mdm) {}

std::shared_ptr<Plan> BetterQueryPlanner::create_plan(
    const parse::QueryData& data,
    std::shared_ptr<tx::Transaction> tx) {

    std::vector<std::shared_ptr<Plan>> plans;
    for (const auto& tblname : data.tables()) {
        auto viewdef = mdm_->get_view_def(tblname, tx);
        if (viewdef.has_value()) {
            parse::Parser parser(viewdef.value());
            auto viewdata = parser.query();
            plans.push_back(create_plan(viewdata, tx));
        } else {
            plans.push_back(std::make_shared<TablePlan>(tx, tblname, mdm_));
        }
    }

    auto p = plans[0];
    for (size_t i = 1; i < plans.size(); i++) {
        // Evaluate both join orders and pick the cheaper one
        auto choice1 = std::make_shared<ProductPlan>(plans[i], p);
        auto choice2 = std::make_shared<ProductPlan>(p, plans[i]);
        if (choice1->blocks_accessed() < choice2->blocks_accessed()) {
            p = choice1;
        } else {
            p = choice2;
        }
    }

    p = std::make_shared<SelectPlan>(p, data.pred());

    return std::make_shared<ProjectPlan>(p, data.fields());
}
