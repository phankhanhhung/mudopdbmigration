#include "plan/tableplan.hpp"
#include "record/tablescan.hpp"
#include "metadata/metadatamgr.hpp"
#include "tx/transaction.hpp"

TablePlan::TablePlan(std::shared_ptr<tx::Transaction> tx,
                     const std::string& tblname,
                     std::shared_ptr<metadata::MetadataMgr> mdm)
    : tblname_(tblname), tx_(tx),
      layout_(mdm->get_layout(tblname, tx)),
      si_(mdm->get_stat_info(tblname, layout_, tx)) {}

std::unique_ptr<Scan> TablePlan::open() {
    return std::make_unique<record::TableScan>(tx_, tblname_, layout_);
}

size_t TablePlan::blocks_accessed() const {
    return si_.blocks_accessed();
}

size_t TablePlan::records_output() const {
    return si_.records_output();
}

size_t TablePlan::distinct_values(const std::string& fldname) const {
    return si_.distinct_values(fldname);
}

std::shared_ptr<record::Schema> TablePlan::schema() const {
    return layout_.schema();
}
