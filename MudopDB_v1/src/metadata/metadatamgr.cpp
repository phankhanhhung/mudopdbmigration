#include "metadata/metadatamgr.hpp"
#include "tx/transaction.hpp"

namespace metadata {

MetadataMgr::MetadataMgr(bool is_new, std::shared_ptr<tx::Transaction> tx)
    : tblmgr_(std::make_shared<TableMgr>(is_new, tx)),
      viewmgr_(is_new, tblmgr_, tx),

void MetadataMgr::create_table(const std::string& tblname,
                                std::shared_ptr<record::Schema> sch,
                                std::shared_ptr<tx::Transaction> tx) {
    tblmgr_->create_table(tblname, sch, tx);
}

record::Layout MetadataMgr::get_layout(const std::string& tblname,
                                        std::shared_ptr<tx::Transaction> tx) {
    return tblmgr_->get_layout(tblname, tx);
}

void MetadataMgr::create_view(const std::string& viewname,
                               const std::string& viewdef,
                               std::shared_ptr<tx::Transaction> tx) {
    viewmgr_.create_view(viewname, viewdef, tx);
}

std::optional<std::string> MetadataMgr::get_view_def(const std::string& viewname,
                                                       std::shared_ptr<tx::Transaction> tx) {
    return viewmgr_.get_view_def(viewname, tx);
}


} // namespace metadata
