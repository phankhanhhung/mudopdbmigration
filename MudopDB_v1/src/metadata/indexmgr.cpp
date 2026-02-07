#include "metadata/indexmgr.hpp"
#include "metadata/tablemgr.hpp"
#include "metadata/statmgr.hpp"
#include "record/tablescan.hpp"
#include "tx/transaction.hpp"

namespace metadata {

IndexMgr::IndexMgr(bool isnew,
                   std::shared_ptr<TableMgr> tblmgr,
                   std::shared_ptr<StatMgr> statmgr,
                   std::shared_ptr<tx::Transaction> tx)
    : layout_(std::make_shared<record::Schema>()), tblmgr_(tblmgr), statmgr_(statmgr) {
    if (isnew) {
        auto sch = std::make_shared<record::Schema>();
        sch->add_string_field("indexname", TableMgr::MAX_NAME);
        sch->add_string_field("tablename", TableMgr::MAX_NAME);
        sch->add_string_field("fieldname", TableMgr::MAX_NAME);
        tblmgr_->create_table("idxcat", sch, tx);
    }
    layout_ = tblmgr_->get_layout("idxcat", tx);
}

void IndexMgr::create_index(const std::string& idxname,
                             const std::string& tblname,
                             const std::string& fldname,
                             std::shared_ptr<tx::Transaction> tx) {
    record::TableScan ts(tx, "idxcat", layout_);
    ts.insert();
    ts.set_string("indexname", idxname);
    ts.set_string("tablename", tblname);
    ts.set_string("fieldname", fldname);
    ts.close();
}

std::unordered_map<std::string, IndexInfo> IndexMgr::get_index_info(
    const std::string& tblname,
    std::shared_ptr<tx::Transaction> tx) {
    std::unordered_map<std::string, IndexInfo> result;
    record::TableScan ts(tx, "idxcat", layout_);
    while (ts.next()) {
        if (ts.get_string("tablename") == tblname) {
            std::string idxname = ts.get_string("indexname");
            std::string fldname = ts.get_string("fieldname");
            auto tbl_layout = tblmgr_->get_layout(tblname, tx);
            auto tblsi = statmgr_->get_stat_info(tblname, tbl_layout, tx);
            IndexInfo ii(idxname, fldname, tbl_layout.schema(), tx, tblsi);
            result.emplace(fldname, ii);
        }
    }
    ts.close();
    return result;
}

} // namespace metadata
