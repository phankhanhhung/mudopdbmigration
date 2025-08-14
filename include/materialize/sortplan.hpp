#ifndef SORTPLAN_HPP
#define SORTPLAN_HPP

#include "plan/plan.hpp"
#include "materialize/recordcomparator.hpp"
#include "materialize/temptable.hpp"
#include <memory>
#include <vector>
#include <string>

namespace tx { class Transaction; }

namespace materialize {

class SortPlan : public Plan {
public:
    SortPlan(std::shared_ptr<tx::Transaction> tx,
             std::shared_ptr<Plan> p,
             const std::vector<std::string>& sortfields);

    std::unique_ptr<Scan> open() override;
    size_t blocks_accessed() const override;
    size_t records_output() const override;
    size_t distinct_values(const std::string& fldname) const override;
    std::shared_ptr<record::Schema> schema() const override;

private:
    std::vector<std::shared_ptr<TempTable>> split_into_runs(Scan& src);
    std::vector<std::shared_ptr<TempTable>> do_a_merge_iteration(
        std::vector<std::shared_ptr<TempTable>>& runs);
    std::shared_ptr<TempTable> merge_two_runs(TempTable& p1, TempTable& p2);
    bool copy(Scan& src, record::TableScan& dest);

    std::shared_ptr<tx::Transaction> tx_;
    std::shared_ptr<Plan> p_;
    std::shared_ptr<record::Schema> sch_;
    RecordComparator comp_;
};

} // namespace materialize

#endif // SORTPLAN_HPP
