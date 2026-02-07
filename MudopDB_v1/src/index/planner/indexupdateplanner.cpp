#include "index/planner/indexupdateplanner.hpp"
#include "metadata/metadatamgr.hpp"
#include "plan/tableplan.hpp"
#include "plan/selectplan.hpp"
#include "query/updatescan.hpp"
#include "record/tablescan.hpp"
#include "parse/insertdata.hpp"
#include "parse/deletedata.hpp"
#include "parse/modifydata.hpp"
#include "parse/createtabledata.hpp"
#include "parse/createviewdata.hpp"
#include "parse/createindexdata.hpp"
#include "tx/transaction.hpp"

namespace index {

IndexUpdatePlanner::IndexUpdatePlanner(std::shared_ptr<metadata::MetadataMgr> mdm)
    : mdm_(mdm) {}

size_t IndexUpdatePlanner::execute_insert(const parse::InsertData& data,
                                           std::shared_ptr<tx::Transaction> tx) {
    std::string tblname = data.table_name();
    auto p = std::make_shared<TablePlan>(tx, tblname, mdm_);

    auto s = p->open();
    auto* us = dynamic_cast<UpdateScan*>(s.get());
    if (!us) return 0;

    us->insert();
    auto rid = us->get_rid().value();

    auto indexes = mdm_->get_index_info(tblname, tx);
    auto fields = data.fields();
    auto vals = data.vals();
    for (size_t i = 0; i < fields.size(); i++) {
        us->set_val(fields[i], vals[i]);

        auto it = indexes.find(fields[i]);
        if (it != indexes.end()) {
            auto idx = it->second.open();
            idx->insert(vals[i], rid);
            idx->close();
        }
    }
    us->close();
    return 1;
}

size_t IndexUpdatePlanner::execute_delete(const parse::DeleteData& data,
                                           std::shared_ptr<tx::Transaction> tx) {
    std::string tblname = data.table_name();
    auto p = std::make_shared<TablePlan>(tx, tblname, mdm_);
    auto sp = std::make_shared<SelectPlan>(p, data.pred());

    auto indexes = mdm_->get_index_info(tblname, tx);

    auto s = sp->open();
    auto* us = dynamic_cast<UpdateScan*>(s.get());
    if (!us) return 0;

    size_t count = 0;
    while (us->next()) {
        auto rid = us->get_rid().value();
        for (auto& [fldname, ii] : indexes) {
            Constant val = us->get_val(fldname);
            auto idx = ii.open();
            idx->delete_entry(val, rid);
            idx->close();
        }
        us->delete_record();
        count++;
    }
    us->close();
    return count;
}

size_t IndexUpdatePlanner::execute_modify(const parse::ModifyData& data,
                                           std::shared_ptr<tx::Transaction> tx) {
    std::string tblname = data.table_name();
    std::string fldname = data.target_field();
    auto p = std::make_shared<TablePlan>(tx, tblname, mdm_);
    auto sp = std::make_shared<SelectPlan>(p, data.pred());

    auto indexes = mdm_->get_index_info(tblname, tx);
    auto it = indexes.find(fldname);
    std::unique_ptr<::Index> idx;
    if (it != indexes.end()) {
        idx = it->second.open();
    }

    auto s = sp->open();
    auto* us = dynamic_cast<UpdateScan*>(s.get());
    if (!us) return 0;

    size_t count = 0;
    while (us->next()) {
        Constant newval = data.new_value().evaluate(*us);
        Constant oldval = us->get_val(fldname);
        us->set_val(fldname, newval);

        if (idx) {
            auto rid = us->get_rid().value();
            idx->delete_entry(oldval, rid);
            idx->insert(newval, rid);
        }
        count++;
    }
    if (idx) idx->close();
    us->close();
    return count;
}

size_t IndexUpdatePlanner::execute_create_table(const parse::CreateTableData& data,
                                                 std::shared_ptr<tx::Transaction> tx) {
    mdm_->create_table(data.table_name(), data.new_schema(), tx);
    return 0;
}

size_t IndexUpdatePlanner::execute_create_view(const parse::CreateViewData& data,
                                                std::shared_ptr<tx::Transaction> tx) {
    mdm_->create_view(data.view_name(), data.view_def(), tx);
    return 0;
}

size_t IndexUpdatePlanner::execute_create_index(const parse::CreateIndexData& data,
                                                 std::shared_ptr<tx::Transaction> tx) {
    mdm_->create_index(data.index_name(), data.table_name(), data.field_name(), tx);
    return 0;
}

} // namespace index
