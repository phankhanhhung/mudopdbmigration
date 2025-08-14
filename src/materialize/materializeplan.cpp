#include "materialize/materializeplan.hpp"
#include "materialize/temptable.hpp"
#include "query/scan.hpp"
#include "query/updatescan.hpp"
#include "record/tablescan.hpp"
#include "record/layout.hpp"
#include "record/schema.hpp"
#include "tx/transaction.hpp"

namespace materialize {

MaterializePlan::MaterializePlan(std::shared_ptr<tx::Transaction> tx,
                                 std::shared_ptr<Plan> srcplan)
    : srcplan_(srcplan), tx_(tx) {}

std::unique_ptr<Scan> MaterializePlan::open() {
    auto sch = srcplan_->schema();
    TempTable temp(tx_, sch);
    auto src = srcplan_->open();
    auto dest = temp.open();
    while (src->next()) {
        dest->insert();
        for (const auto& fldname : sch->fields()) {
            dest->set_val(fldname, src->get_val(fldname));
        }
    }
    src->close();
    dest->before_first();
    return dest;
}

size_t MaterializePlan::blocks_accessed() const {
    record::Layout layout(srcplan_->schema());
    size_t rpb = tx_->block_size() / layout.slot_size();
    if (rpb == 0) rpb = 1;
    size_t numrecs = srcplan_->records_output();
    return (numrecs + rpb - 1) / rpb;
}

size_t MaterializePlan::records_output() const {
    return srcplan_->records_output();
}

size_t MaterializePlan::distinct_values(const std::string& fldname) const {
    return srcplan_->distinct_values(fldname);
}

std::shared_ptr<record::Schema> MaterializePlan::schema() const {
    return srcplan_->schema();
}

} // namespace materialize
