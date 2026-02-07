#include "metadata/metadatamgr.hpp"
#include "tx/transaction.hpp"

namespace metadata {

MetadataMgr::MetadataMgr(bool is_new, std::shared_ptr<tx::Transaction> tx)
    : tblmgr_(std::make_shared<TableMgr>(is_new, tx)),
      viewmgr_(is_new, tblmgr_, tx),
      statmgr_(std::make_shared<StatMgr>(tblmgr_, tx)),
      idxmgr_(is_new, tblmgr_, statmgr_, tx) {}

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

void MetadataMgr::create_index(const std::string& idxname,
                                const std::string& tblname,
                                const std::string& fldname,
                                std::shared_ptr<tx::Transaction> tx) {
    idxmgr_.create_index(idxname, tblname, fldname, tx);
}

std::unordered_map<std::string, IndexInfo> MetadataMgr::get_index_info(
    const std::string& tblname,
    std::shared_ptr<tx::Transaction> tx) {
    return idxmgr_.get_index_info(tblname, tx);
}

StatInfo MetadataMgr::get_stat_info(const std::string& tblname,
                                     const record::Layout& layout,
                                     std::shared_ptr<tx::Transaction> tx) {
    return statmgr_->get_stat_info(tblname, layout, tx);
}

} // namespace metadata
