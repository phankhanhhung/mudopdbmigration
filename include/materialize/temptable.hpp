#ifndef TEMPTABLE_HPP
#define TEMPTABLE_HPP

#include "record/layout.hpp"
#include "record/schema.hpp"
#include "record/tablescan.hpp"
#include <memory>
#include <string>
#include <atomic>

namespace tx { class Transaction; }

namespace materialize {

class TempTable {
public:
    TempTable(std::shared_ptr<tx::Transaction> tx, std::shared_ptr<record::Schema> sch);

    std::unique_ptr<record::TableScan> open();
    std::string table_name() const;
    record::Layout get_layout() const;

private:
    static std::string next_table_name();

    std::shared_ptr<tx::Transaction> tx_;
    std::string tblname_;
    record::Layout layout_;
};

} // namespace materialize

#endif // TEMPTABLE_HPP
