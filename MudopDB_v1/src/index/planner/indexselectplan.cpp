#include "index/planner/indexselectplan.hpp"
#include "index/query/indexselectscan.hpp"
#include "record/tablescan.hpp"
#include <stdexcept>

namespace index {

IndexSelectPlan::IndexSelectPlan(std::shared_ptr<Plan> p,
                                 const metadata::IndexInfo& ii,
                                 const Constant& val)
    : p_(p), ii_(ii), val_(val) {}

std::unique_ptr<Scan> IndexSelectPlan::open() {
    auto s = p_->open();
    auto* ts = dynamic_cast<record::TableScan*>(s.get());
    if (!ts) {
        throw std::runtime_error("IndexSelectPlan requires TableScan");
    }
    s.release();
    auto ts_ptr = std::unique_ptr<record::TableScan>(ts);
    auto idx = ii_.open();
    return std::make_unique<IndexSelectScan>(std::move(ts_ptr), std::move(idx), val_);
}

size_t IndexSelectPlan::blocks_accessed() const {
    return ii_.blocks_accessed() + records_output();
}

size_t IndexSelectPlan::records_output() const {
    return ii_.records_output();
}

size_t IndexSelectPlan::distinct_values(const std::string& fldname) const {
    return ii_.distinct_values(fldname);
}

std::shared_ptr<record::Schema> IndexSelectPlan::schema() const {
    return p_->schema();
}

} // namespace index
