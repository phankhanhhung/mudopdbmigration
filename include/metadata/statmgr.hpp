/**
 * @file statmgr.hpp
 * @brief Statistics manager with caching and periodic refresh.
 */

#ifndef STATMGR_HPP
#define STATMGR_HPP

#include "metadata/tablemgr.hpp"
#include "metadata/statinfo.hpp"
#include "record/layout.hpp"
#include <memory>
#include <unordered_map>
#include <string>

namespace tx {
class Transaction;
}

namespace metadata {

/**
 * StatMgr manages table statistics.
 * Refreshes statistics after every 100 calls.
 *
 * Corresponds to StatMgr in Rust (NMDB2/src/metadata/statmgr.rs)
 */
class StatMgr {
public:
    StatMgr(std::shared_ptr<TableMgr> tbl_mgr,
            std::shared_ptr<tx::Transaction> tx);

    StatInfo get_stat_info(const std::string& tblname,
                           const record::Layout& layout,
                           std::shared_ptr<tx::Transaction> tx);

private:
    void refresh_statistics(std::shared_ptr<tx::Transaction> tx);
    StatInfo calc_table_stats(const std::string& tblname,
                              const record::Layout& layout,
                              std::shared_ptr<tx::Transaction> tx);

    std::shared_ptr<TableMgr> tbl_mgr_;
    std::unordered_map<std::string, StatInfo> tablestats_;
    size_t numcalls_;
};

} // namespace metadata

#endif // STATMGR_HPP
