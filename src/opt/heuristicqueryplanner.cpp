/**
 * HeuristicQueryPlanner implementation.
 * Uses heuristic rules to choose join order: smallest table first,
 * apply selections early, use indexes when available.
 */

#include "opt/heuristicqueryplanner.hpp"
#include "plan/projectplan.hpp"
#include "parse/querydata.hpp"
#include "metadata/metadatamgr.hpp"
#include "tx/transaction.hpp"
#include <stdexcept>

namespace opt {

HeuristicQueryPlanner::HeuristicQueryPlanner(std::shared_ptr<metadata::MetadataMgr> mdm)
    : mdm_(mdm) {}

std::shared_ptr<Plan> HeuristicQueryPlanner::create_plan(
    const parse::QueryData& data,
    std::shared_ptr<tx::Transaction> tx) {

    tableplanners_.clear();
    for (const auto& tblname : data.tables()) {
        tableplanners_.emplace_back(tblname, data.pred(), tx, mdm_);
    }

    auto currentplan = get_lowest_select_plan();

    while (!tableplanners_.empty()) {
        if (!currentplan) {
            throw std::runtime_error("HeuristicQueryPlanner: no plan available");
        }
        auto p = get_lowest_join_plan(currentplan);
        if (p) {
            currentplan = p;
        } else {
            currentplan = get_lowest_product_plan(currentplan);
        }
    }

    if (!currentplan) {
        throw std::runtime_error("HeuristicQueryPlanner: no plan available");
    }
    return std::make_shared<ProjectPlan>(currentplan, data.fields());
}

std::shared_ptr<Plan> HeuristicQueryPlanner::get_lowest_select_plan() {
    size_t bestidx = 0;
    std::shared_ptr<Plan> bestplan = nullptr;
    for (size_t idx = 0; idx < tableplanners_.size(); idx++) {
        auto plan = tableplanners_[idx].make_select_plan();
        if (!bestplan || plan->records_output() < bestplan->records_output()) {
            bestidx = idx;
            bestplan = plan;
        }
    }
    if (bestplan) {
        tableplanners_.erase(tableplanners_.begin() + bestidx);
    }
    return bestplan;
}

std::shared_ptr<Plan> HeuristicQueryPlanner::get_lowest_join_plan(
    const std::shared_ptr<Plan>& current) {
    size_t bestidx = 0;
    std::shared_ptr<Plan> bestplan = nullptr;
    for (size_t idx = 0; idx < tableplanners_.size(); idx++) {
        auto plan = tableplanners_[idx].make_join_plan(current);
        if (plan) {
            if (!bestplan || plan->records_output() < bestplan->records_output()) {
                bestidx = idx;
                bestplan = plan;
            }
        }
    }
    if (bestplan) {
        tableplanners_.erase(tableplanners_.begin() + bestidx);
    }
    return bestplan;
}

std::shared_ptr<Plan> HeuristicQueryPlanner::get_lowest_product_plan(
    const std::shared_ptr<Plan>& current) {
    size_t bestidx = 0;
    std::shared_ptr<Plan> bestplan = nullptr;
    for (size_t idx = 0; idx < tableplanners_.size(); idx++) {
        auto plan = tableplanners_[idx].make_product_plan(current);
        if (!bestplan || plan->records_output() < bestplan->records_output()) {
            bestidx = idx;
            bestplan = plan;
        }
    }
    if (bestplan) {
        tableplanners_.erase(tableplanners_.begin() + bestidx);
    }
    return bestplan;
}

} // namespace opt
