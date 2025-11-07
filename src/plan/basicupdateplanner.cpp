/**
 * BasicUpdatePlanner implementation.
 * Executes INSERT, DELETE, MODIFY, CREATE TABLE, CREATE VIEW, CREATE INDEX.
 */

#include "plan/basicupdateplanner.hpp"
#include "plan/tableplan.hpp"
#include "plan/selectplan.hpp"
#include "parse/insertdata.hpp"
#include "parse/deletedata.hpp"
#include "parse/modifydata.hpp"
#include "parse/createtabledata.hpp"
#include "parse/createviewdata.hpp"
#include "parse/createindexdata.hpp"
#include "query/updatescan.hpp"
#include "record/tablescan.hpp"
#include "metadata/metadatamgr.hpp"
#include "tx/transaction.hpp"

BasicUpdatePlanner::BasicUpdatePlanner(std::shared_ptr<metadata::MetadataMgr> mdm)
    : mdm_(mdm) {}

size_t BasicUpdatePlanner::execute_delete(
    const parse::DeleteData& data,
    std::shared_ptr<tx::Transaction> tx) {

    std::shared_ptr<Plan> p = std::make_shared<TablePlan>(tx, data.table_name(), mdm_);
    p = std::make_shared<SelectPlan>(p, data.pred());
    auto s = p->open();
    auto us = dynamic_cast<UpdateScan*>(s.get());
    if (!us) throw std::runtime_error("Expected UpdateScan for delete");
    size_t count = 0;
    while (us->next()) {
        us->delete_record();
        count++;
    }
    us->close();
    return count;
}

size_t BasicUpdatePlanner::execute_modify(
    const parse::ModifyData& data,
    std::shared_ptr<tx::Transaction> tx) {

    std::shared_ptr<Plan> p = std::make_shared<TablePlan>(tx, data.table_name(), mdm_);
    p = std::make_shared<SelectPlan>(p, data.pred());
    auto s = p->open();
    auto us = dynamic_cast<UpdateScan*>(s.get());
    if (!us) throw std::runtime_error("Expected UpdateScan for modify");
    size_t count = 0;
    while (us->next()) {
        Constant val = data.new_value().evaluate(*us);
        us->set_val(data.target_field(), val);
        count++;
    }
    us->close();
    return count;
}

size_t BasicUpdatePlanner::execute_insert(
    const parse::InsertData& data,
    std::shared_ptr<tx::Transaction> tx) {

    auto p = std::make_shared<TablePlan>(tx, data.table_name(), mdm_);
    auto s = p->open();
    auto us = dynamic_cast<UpdateScan*>(s.get());
    if (!us) throw std::runtime_error("Expected UpdateScan for insert");
    us->insert();
    auto vals = data.vals();
    auto flds = data.fields();
    auto it = vals.begin();
    for (const auto& fldname : flds) {
        if (it != vals.end()) {
            us->set_val(fldname, *it);
            ++it;
        }
    }
    us->close();
    return 1;
}

size_t BasicUpdatePlanner::execute_create_table(
    const parse::CreateTableData& data,
    std::shared_ptr<tx::Transaction> tx) {
    mdm_->create_table(data.table_name(), data.new_schema(), tx);
    return 0;
}

size_t BasicUpdatePlanner::execute_create_view(
    const parse::CreateViewData& data,
    std::shared_ptr<tx::Transaction> tx) {
    mdm_->create_view(data.view_name(), data.view_def(), tx);
    return 0;
}

size_t BasicUpdatePlanner::execute_create_index(
    const parse::CreateIndexData& data,
    std::shared_ptr<tx::Transaction> tx) {
    // Index creation deferred to Phase 7
    (void)data;
    (void)tx;
    return 0;
}
