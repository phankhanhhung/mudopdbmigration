/**
 * @file viewmgr.hpp
 * @brief System catalog manager for view definitions.
 */

#ifndef VIEWMGR_HPP
#define VIEWMGR_HPP

#include "metadata/tablemgr.hpp"
#include <memory>
#include <optional>
#include <string>

namespace tx {
class Transaction;
}

namespace metadata {

/**
 * ViewMgr manages view definitions via viewcat table.
 *
 * Corresponds to ViewMgr in Rust (NMDB2/src/metadata/viewmgr.rs)
 */
class ViewMgr {
public:
    ViewMgr(bool is_new,
            std::shared_ptr<TableMgr> tbl_mgr,
            std::shared_ptr<tx::Transaction> tx);

    void create_view(const std::string& vname,
                     const std::string& vdef,
                     std::shared_ptr<tx::Transaction> tx);

    std::optional<std::string> get_view_def(const std::string& vname,
                                             std::shared_ptr<tx::Transaction> tx);

private:
    static constexpr size_t MAX_VIEWDEF = 100;
    std::shared_ptr<TableMgr> tbl_mgr_;
};

} // namespace metadata

#endif // VIEWMGR_HPP
