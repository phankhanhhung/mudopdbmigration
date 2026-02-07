#ifndef METADATAMGR_HPP
#define METADATAMGR_HPP

#include "metadata/tablemgr.hpp"
#include "metadata/viewmgr.hpp"
#include "metadata/statmgr.hpp"
#include "metadata/statinfo.hpp"
#include "metadata/indexmgr.hpp"
#include "record/layout.hpp"
#include "record/schema.hpp"
#include <memory>
#include <string>
#include <optional>
#include <unordered_map>

namespace tx {
class Transaction;
}

namespace metadata {

/**
 * MetadataMgr provides a unified interface to all metadata managers.
 *
 * Corresponds to MetadataMgr in Rust (NMDB2/src/metadata/metadatamgr.rs)
 */
class MetadataMgr {
public:
    MetadataMgr(bool is_new, std::shared_ptr<tx::Transaction> tx);

    void create_table(const std::string& tblname,
                      std::shared_ptr<record::Schema> sch,
                      std::shared_ptr<tx::Transaction> tx);

    record::Layout get_layout(const std::string& tblname,
                              std::shared_ptr<tx::Transaction> tx);

    void create_view(const std::string& viewname,
                     const std::string& viewdef,
                     std::shared_ptr<tx::Transaction> tx);

    std::optional<std::string> get_view_def(const std::string& viewname,
                                             std::shared_ptr<tx::Transaction> tx);

    void create_index(const std::string& idxname,
                      const std::string& tblname,
                      const std::string& fldname,
                      std::shared_ptr<tx::Transaction> tx);

    std::unordered_map<std::string, IndexInfo> get_index_info(
        const std::string& tblname,
        std::shared_ptr<tx::Transaction> tx);

    StatInfo get_stat_info(const std::string& tblname,
                           const record::Layout& layout,
                           std::shared_ptr<tx::Transaction> tx);

private:
    std::shared_ptr<TableMgr> tblmgr_;
    ViewMgr viewmgr_;
    std::shared_ptr<StatMgr> statmgr_;
    IndexMgr idxmgr_;
};

} // namespace metadata

#endif // METADATAMGR_HPP
