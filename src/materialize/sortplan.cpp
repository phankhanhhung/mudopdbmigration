#include "materialize/sortplan.hpp"
#include "materialize/sortscan.hpp"
#include "materialize/materializeplan.hpp"
#include "query/scan.hpp"
#include "query/updatescan.hpp"
#include "record/tablescan.hpp"
#include "record/schema.hpp"
#include "tx/transaction.hpp"

namespace materialize {

SortPlan::SortPlan(std::shared_ptr<tx::Transaction> tx,
                   std::shared_ptr<Plan> p,
                   const std::vector<std::string>& sortfields)
    : tx_(tx), p_(p), sch_(p->schema()), comp_(sortfields) {}

std::unique_ptr<Scan> SortPlan::open() {
    auto src = p_->open();
    auto runs = split_into_runs(*src);
    src->close();
    while (runs.size() > 2) {
        runs = do_a_merge_iteration(runs);
    }
    return std::make_unique<SortScan>(runs, comp_);
}

size_t SortPlan::blocks_accessed() const {
    MaterializePlan mp(tx_, p_);
    return mp.blocks_accessed();
}

size_t SortPlan::records_output() const {
    return p_->records_output();
}

size_t SortPlan::distinct_values(const std::string& fldname) const {
    return p_->distinct_values(fldname);
}

std::shared_ptr<record::Schema> SortPlan::schema() const {
    return sch_;
}

std::vector<std::shared_ptr<TempTable>> SortPlan::split_into_runs(Scan& src) {
    std::vector<std::shared_ptr<TempTable>> temps;
    src.before_first();
    if (!src.next()) {
        return temps;
    }
    auto currenttemp = std::make_shared<TempTable>(tx_, sch_);
    temps.push_back(currenttemp);
    auto currentscan = currenttemp->open();
    while (copy(src, *currentscan)) {
        if (comp_.compare(src, *currentscan) < 0) {
            currentscan->close();
            currenttemp = std::make_shared<TempTable>(tx_, sch_);
            temps.push_back(currenttemp);
            currentscan = currenttemp->open();
        }
    }
    currentscan->close();
    return temps;
}

std::vector<std::shared_ptr<TempTable>> SortPlan::do_a_merge_iteration(
    std::vector<std::shared_ptr<TempTable>>& runs) {
    std::vector<std::shared_ptr<TempTable>> result;
    while (runs.size() > 1) {
        auto p1 = runs[0];
        auto p2 = runs[1];
        runs.erase(runs.begin(), runs.begin() + 2);
        result.push_back(merge_two_runs(*p1, *p2));
    }
    if (runs.size() == 1) {
        result.push_back(runs[0]);
    }
    return result;
}

std::shared_ptr<TempTable> SortPlan::merge_two_runs(TempTable& p1, TempTable& p2) {
    auto src1 = p1.open();
    auto src2 = p2.open();
    auto result = std::make_shared<TempTable>(tx_, sch_);
    auto dest = result->open();

    bool hasmore1 = src1->next();
    bool hasmore2 = src2->next();
    while (hasmore1 && hasmore2) {
        if (comp_.compare(*src1, *src2) < 0) {
            hasmore1 = copy(*src1, *dest);
        } else {
            hasmore2 = copy(*src2, *dest);
        }
    }
    while (hasmore1) {
        hasmore1 = copy(*src1, *dest);
    }
    while (hasmore2) {
        hasmore2 = copy(*src2, *dest);
    }
    src1->close();
    src2->close();
    dest->close();
    return result;
}

bool SortPlan::copy(Scan& src, record::TableScan& dest) {
    dest.insert();
    for (const auto& fldname : sch_->fields()) {
        dest.set_val(fldname, src.get_val(fldname));
    }
    return src.next();
}

} // namespace materialize
