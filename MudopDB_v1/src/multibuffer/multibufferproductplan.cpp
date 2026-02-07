#include "multibuffer/multibufferproductplan.hpp"
#include "multibuffer/multibufferproductscan.hpp"
#include "materialize/materializeplan.hpp"
#include "materialize/temptable.hpp"
#include "query/updatescan.hpp"
#include "record/tablescan.hpp"
#include "tx/transaction.hpp"

namespace multibuffer {

MultibufferProductPlan::MultibufferProductPlan(std::shared_ptr<tx::Transaction> tx,
                                               std::shared_ptr<Plan> lhs,
                                               std::shared_ptr<Plan> rhs)
    : tx_(tx), rhs_(rhs) {
    // Materialize LHS
    lhs_ = std::make_shared<materialize::MaterializePlan>(tx, lhs);
    schema_ = std::make_shared<record::Schema>();
    schema_->add_all(*lhs_->schema());
    schema_->add_all(*rhs_->schema());
}

static void copy_records_from(std::shared_ptr<tx::Transaction> tx,
                               std::shared_ptr<Plan> p,
                               materialize::TempTable& tt) {
    auto src = p->open();
    auto dest = tt.open();
    while (src->next()) {
        dest->insert();
        for (const auto& fldname : p->schema()->fields()) {
            dest->set_val(fldname, src->get_val(fldname));
        }
    }
    src->close();
    dest->close();
}

std::unique_ptr<Scan> MultibufferProductPlan::open() {
    auto leftscan = lhs_->open();
    materialize::TempTable tt(tx_, rhs_->schema());
    copy_records_from(tx_, rhs_, tt);
    return std::make_unique<MultibufferProductScan>(tx_, std::move(leftscan),
                                                     tt.table_name(), tt.get_layout());
}

size_t MultibufferProductPlan::blocks_accessed() const {
    auto mp = std::make_shared<materialize::MaterializePlan>(tx_, rhs_);
    size_t avail = tx_->available_buffs();
    if (avail == 0) avail = 1;
    size_t size = mp->blocks_accessed();
    size_t numchunks = (size + avail - 1) / avail;
    return rhs_->blocks_accessed() + (lhs_->blocks_accessed() * numchunks);
}

size_t MultibufferProductPlan::records_output() const {
    return lhs_->records_output() * rhs_->records_output();
}

size_t MultibufferProductPlan::distinct_values(const std::string& fldname) const {
    if (lhs_->schema()->has_field(fldname)) {
        return lhs_->distinct_values(fldname);
    }
    return rhs_->distinct_values(fldname);
}

std::shared_ptr<record::Schema> MultibufferProductPlan::schema() const {
    return schema_;
}

} // namespace multibuffer
