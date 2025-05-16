#ifndef TABLEMGR_HPP
#define TABLEMGR_HPP

#include "record/layout.hpp"
#include "record/schema.hpp"
#include <memory>
#include <string>

namespace tx {
class Transaction;
}

namespace metadata {

class TableMgr {
public:
    static constexpr size_t MAX_NAME = 16;

    TableMgr(bool is_new, std::shared_ptr<tx::Transaction> tx);

    void create_table(const std::string& tblname,
                      std::shared_ptr<record::Schema> sch,
                      std::shared_ptr<tx::Transaction> tx);

    record::Layout get_layout(const std::string& tblname,
                              std::shared_ptr<tx::Transaction> tx);

private:
    record::Layout tcat_layout_;
    record::Layout fcat_layout_;
};

} // namespace metadata

#endif // TABLEMGR_HPP
