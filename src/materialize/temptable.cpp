#include "materialize/temptable.hpp"
#include "tx/transaction.hpp"
#include <atomic>

namespace materialize {

static std::atomic<size_t> NEXT_TABLE_NUM{0};

std::string TempTable::next_table_name() {
    NEXT_TABLE_NUM.fetch_add(1, std::memory_order_seq_cst);
    return "temp" + std::to_string(NEXT_TABLE_NUM.load(std::memory_order_seq_cst));
}

TempTable::TempTable(std::shared_ptr<tx::Transaction> tx, std::shared_ptr<record::Schema> sch)
    : tx_(tx), tblname_(next_table_name()), layout_(sch) {}

std::unique_ptr<record::TableScan> TempTable::open() {
    return std::make_unique<record::TableScan>(tx_, tblname_, layout_);
}

std::string TempTable::table_name() const {
    return tblname_;
}

record::Layout TempTable::get_layout() const {
    return layout_;
}

} // namespace materialize
