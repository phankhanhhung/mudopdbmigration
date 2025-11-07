/**
 * BasicQueryPlanner implementation.
 * Creates a plan tree by taking the product of all tables, then applying
 * selection and projection. No join optimization.
 */

#include "plan/basicqueryplanner.hpp"
#include "plan/tableplan.hpp"
#include "plan/selectplan.hpp"
#include "plan/projectplan.hpp"
#include "plan/productplan.hpp"
#include "parse/querydata.hpp"
#include "parse/parser.hpp"
#include "metadata/metadatamgr.hpp"
#include "tx/transaction.hpp"

BasicQueryPlanner::BasicQueryPlanner(std::shared_ptr<metadata::MetadataMgr> mdm)
    : mdm_(mdm) {}

std::shared_ptr<Plan> BasicQueryPlanner::create_plan(
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
        p = std::make_shared<ProductPlan>(p, plans[i]);
    }

    p = std::make_shared<SelectPlan>(p, data.pred());

    return std::make_shared<ProjectPlan>(p, data.fields());
}
