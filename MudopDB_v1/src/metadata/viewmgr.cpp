#include "metadata/viewmgr.hpp"
#include "record/tablescan.hpp"
#include "tx/transaction.hpp"

namespace metadata {

ViewMgr::ViewMgr(bool is_new,
                 std::shared_ptr<TableMgr> tbl_mgr,
                 std::shared_ptr<tx::Transaction> tx)
    : tbl_mgr_(tbl_mgr) {
    if (is_new) {
        auto sch = std::make_shared<record::Schema>();
        sch->add_string_field("viewname", TableMgr::MAX_NAME);
        sch->add_string_field("viewdef", MAX_VIEWDEF);
        tbl_mgr_->create_table("viewcat", sch, tx);
    }
}

void ViewMgr::create_view(const std::string& vname,
                           const std::string& vdef,
                           std::shared_ptr<tx::Transaction> tx) {
    record::Layout layout = tbl_mgr_->get_layout("viewcat", tx);
    record::TableScan ts(tx, "viewcat", layout);
    ts.insert();
    ts.set_string("viewname", vname);
    ts.set_string("viewdef", vdef);
    ts.close();
}

std::optional<std::string> ViewMgr::get_view_def(const std::string& vname,
                                                   std::shared_ptr<tx::Transaction> tx) {
    record::Layout layout = tbl_mgr_->get_layout("viewcat", tx);
    record::TableScan ts(tx, "viewcat", layout);
    while (ts.next()) {
        if (ts.get_string("viewname") == vname) {
            std::string result = ts.get_string("viewdef");
            ts.close();
            return result;
        }
    }
    ts.close();
    return std::nullopt;
}

} // namespace metadata
