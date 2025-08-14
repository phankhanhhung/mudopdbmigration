#include "opt/tableplanner.hpp"
#include "plan/selectplan.hpp"
#include "plan/productplan.hpp"
#include "metadata/metadatamgr.hpp"
#include "tx/transaction.hpp"
#include "record/schema.hpp"

namespace opt {

TablePlanner::TablePlanner(const std::string& tblname,
                           const Predicate& mypred,
                           std::shared_ptr<tx::Transaction> tx,
                           std::shared_ptr<metadata::MetadataMgr> mdm)
    : mypred_(mypred), tx_(tx) {
    myplan_ = std::make_shared<::TablePlan>(tx, tblname, mdm);
    myschema_ = myplan_->schema();
}

std::shared_ptr<Plan> TablePlanner::make_select_plan() const {
    // No index support yet (Phase 7)
    return add_select_pred(myplan_);
}

std::shared_ptr<Plan> TablePlanner::make_join_plan(
    const std::shared_ptr<Plan>& current) const {
    auto currsch = current->schema();
    auto joinpred = mypred_.join_sub_pred(myschema_, currsch);
    if (!joinpred.has_value()) {
        return nullptr;
    }
    // No index join support yet - use product join
    auto p = make_product_plan(current);
    return add_join_pred(p, currsch);
}

std::shared_ptr<Plan> TablePlanner::make_product_plan(
    const std::shared_ptr<Plan>& current) const {
    auto p = add_select_pred(myplan_);
    return std::make_shared<ProductPlan>(current, p);
}

std::shared_ptr<Plan> TablePlanner::add_select_pred(std::shared_ptr<Plan> p) const {
    auto selectpred = mypred_.select_sub_pred(myschema_);
    if (selectpred.has_value()) {
        return std::make_shared<SelectPlan>(p, selectpred.value());
    }
    return p;
}

std::shared_ptr<Plan> TablePlanner::add_join_pred(
    std::shared_ptr<Plan> p, std::shared_ptr<record::Schema> currsch) const {
    auto joinpred = mypred_.join_sub_pred(currsch, myschema_);
    if (joinpred.has_value()) {
        return std::make_shared<SelectPlan>(p, joinpred.value());
    }
    return p;
}

} // namespace opt
