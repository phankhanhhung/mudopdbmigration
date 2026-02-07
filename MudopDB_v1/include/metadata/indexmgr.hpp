#ifndef INDEXMGR_HPP
#define INDEXMGR_HPP

#include "metadata/indexinfo.hpp"
#include "record/layout.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace metadata {

class TableMgr;
class StatMgr;

/**
 * IndexMgr manages index metadata using the idxcat catalog table.
 *
 * Corresponds to IndexMgr in Rust (NMDB2/src/metadata/indexmgr.rs)
 */
class IndexMgr {
public:
    IndexMgr(bool isnew,
             std::shared_ptr<TableMgr> tblmgr,
             std::shared_ptr<StatMgr> statmgr,
             std::shared_ptr<tx::Transaction> tx);

    void create_index(const std::string& idxname,
                      const std::string& tblname,
                      const std::string& fldname,
                      std::shared_ptr<tx::Transaction> tx);

    std::unordered_map<std::string, IndexInfo> get_index_info(
        const std::string& tblname,
        std::shared_ptr<tx::Transaction> tx);

private:
    record::Layout layout_;
    std::shared_ptr<TableMgr> tblmgr_;
    std::shared_ptr<StatMgr> statmgr_;
};

} // namespace metadata

#endif // INDEXMGR_HPP
