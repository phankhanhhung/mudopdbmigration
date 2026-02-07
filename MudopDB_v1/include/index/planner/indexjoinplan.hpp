#ifndef INDEXJOINPLAN_HPP
#define INDEXJOINPLAN_HPP

#include "plan/plan.hpp"
#include "metadata/indexinfo.hpp"
#include "record/schema.hpp"
#include <memory>
#include <string>

namespace index {

/**
 * Plan that uses an index for join.
 *
 * Corresponds to IndexJoinPlan in Rust (NMDB2/src/index/planner/indexjoinplan.rs)
 */
class IndexJoinPlan : public Plan {
public:
    IndexJoinPlan(std::shared_ptr<Plan> p1,
                  std::shared_ptr<Plan> p2,
                  const metadata::IndexInfo& ii,
                  const std::string& joinfield);

    std::unique_ptr<Scan> open() override;
    size_t blocks_accessed() const override;
    size_t records_output() const override;
    size_t distinct_values(const std::string& fldname) const override;
    std::shared_ptr<record::Schema> schema() const override;

private:
    std::shared_ptr<Plan> p1_;
    std::shared_ptr<Plan> p2_;
    metadata::IndexInfo ii_;
    std::string joinfield_;
    std::shared_ptr<record::Schema> sch_;
};

} // namespace index

#endif // INDEXJOINPLAN_HPP
