#include "opt/tableplanner.hpp"
#include "metadata/metadatamgr.hpp"
#include "plan/selectplan.hpp"
#include "plan/productplan.hpp"
#include "index/planner/indexselectplan.hpp"
#include "index/planner/indexjoinplan.hpp"
#include "multibuffer/multibufferproductplan.hpp"
#include "tx/transaction.hpp"

namespace opt {

TablePlanner::TablePlanner(const std::string& tblname,
                           const Predicate& mypred,
                           std::shared_ptr<tx::Transaction> tx,
                           std::shared_ptr<metadata::MetadataMgr> mdm)
    : myplan_(std::make_shared<::TablePlan>(tx, tblname, mdm)),
      mypred_(mypred),
      myschema_(myplan_->schema()),
      indexes_(mdm->get_index_info(tblname, tx)),
      tx_(tx) {}

std::shared_ptr<Plan> TablePlanner::make_select_plan() const {
    auto p = make_index_select();
    if (!p) {
        p = myplan_;
    }
    return add_select_pred(p);
}

std::shared_ptr<Plan> TablePlanner::make_join_plan(const std::shared_ptr<Plan>& current) const {
    auto currsch = current->schema();
    auto joinpred = mypred_.join_sub_pred(myschema_, currsch);
    if (!joinpred.has_value()) {
        return nullptr;
    }
    auto p = make_index_join(current, currsch);
    if (p) {
        return p;
    }
    return make_product_join(current, currsch);
}

std::shared_ptr<Plan> TablePlanner::make_product_plan(const std::shared_ptr<Plan>& current) const {
    auto p = add_select_pred(myplan_);
    return std::make_shared<multibuffer::MultibufferProductPlan>(tx_, current, p);
}

std::shared_ptr<Plan> TablePlanner::make_index_select() const {
    for (const auto& [fldname, ii] : indexes_) {
        auto val = mypred_.equates_with_constant(fldname);
        if (val.has_value()) {
            return std::make_shared<index::IndexSelectPlan>(myplan_, ii, val.value());
        }
    }
    return nullptr;
}

std::shared_ptr<Plan> TablePlanner::make_index_join(const std::shared_ptr<Plan>& current,
                                                     std::shared_ptr<record::Schema> currsch) const {
    for (const auto& [fldname, ii] : indexes_) {
        auto outerfield = mypred_.equates_with_field(fldname);
        if (outerfield.has_value() && currsch->has_field(outerfield.value())) {
            auto p = std::make_shared<index::IndexJoinPlan>(current, myplan_, ii, outerfield.value());
            auto p2 = add_select_pred(p);
            return add_join_pred(p2, currsch);
        }
    }
    return nullptr;
}

std::shared_ptr<Plan> TablePlanner::make_product_join(const std::shared_ptr<Plan>& current,
                                                       std::shared_ptr<record::Schema> currsch) const {
    auto p = make_product_plan(current);
    return add_join_pred(p, currsch);
}

std::shared_ptr<Plan> TablePlanner::add_select_pred(std::shared_ptr<Plan> p) const {
    auto selectpred = mypred_.select_sub_pred(myschema_);
    if (selectpred.has_value()) {
        return std::make_shared<SelectPlan>(p, selectpred.value());
    }
    return p;
}

std::shared_ptr<Plan> TablePlanner::add_join_pred(std::shared_ptr<Plan> p,
                                                   std::shared_ptr<record::Schema> currsch) const {
    auto joinpred = mypred_.join_sub_pred(currsch, myschema_);
    if (joinpred.has_value()) {
        return std::make_shared<SelectPlan>(p, joinpred.value());
    }
    return p;
}

} // namespace opt
