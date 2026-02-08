#include "metadata/statmgr.hpp"
#include "record/tablescan.hpp"
#include "tx/transaction.hpp"

namespace metadata {

StatMgr::StatMgr(std::shared_ptr<TableMgr> tbl_mgr,
                 std::shared_ptr<tx::Transaction> tx)
    : tbl_mgr_(tbl_mgr), numcalls_(0) {
    refresh_statistics(tx);
}

StatInfo StatMgr::get_stat_info(const std::string& tblname,
                                 const record::Layout& layout,
                                 std::shared_ptr<tx::Transaction> tx) {
    std::lock_guard<std::mutex> lock(mutex_);
    numcalls_++;
    if (numcalls_ > 100) {
        refresh_statistics(tx);
    }
    auto it = tablestats_.find(tblname);
    if (it != tablestats_.end()) {
        return it->second;
    }
    StatInfo si = calc_table_stats(tblname, layout, tx);
    tablestats_.emplace(tblname, si);
    return si;
}

void StatMgr::refresh_statistics(std::shared_ptr<tx::Transaction> tx) {
    tablestats_.clear();
    numcalls_ = 0;
    record::Layout tcatlayout = tbl_mgr_->get_layout("tblcat", tx);
    record::TableScan tcat(tx, "tblcat", tcatlayout);
    while (tcat.next().value()) {
        std::string tblname = tcat.get_string("tblname").value();
        record::Layout layout = tbl_mgr_->get_layout(tblname, tx);
        StatInfo si = calc_table_stats(tblname, layout, tx);
        tablestats_.emplace(tblname, si);
    }
    tcat.close().value();
}

StatInfo StatMgr::calc_table_stats(const std::string& tblname,
                                    const record::Layout& layout,
                                    std::shared_ptr<tx::Transaction> tx) {
    size_t num_recs = 0;
    size_t numblocks = 0;
    record::TableScan ts(tx, tblname, layout);
    while (ts.next().value()) {
        num_recs++;
        auto rid = ts.get_rid();
        if (rid.has_value()) {
            numblocks = static_cast<size_t>(rid.value().block_number()) + 1;
        }
    }
    ts.close().value();
    return StatInfo(numblocks, num_recs);
}

} // namespace metadata
