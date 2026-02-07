#ifndef INDEXSELECTPLAN_HPP
#define INDEXSELECTPLAN_HPP

#include "plan/plan.hpp"
#include "metadata/indexinfo.hpp"
#include "query/constant.hpp"
#include "record/schema.hpp"
#include <memory>
#include <string>

namespace index {

/**
 * Plan that uses an index for selection (equality).
 *
 * Corresponds to IndexSelectPlan in Rust (NMDB2/src/index/planner/indexselectplan.rs)
 */
class IndexSelectPlan : public Plan {
public:
    IndexSelectPlan(std::shared_ptr<Plan> p,
                    const metadata::IndexInfo& ii,
                    const Constant& val);

    std::unique_ptr<Scan> open() override;
    size_t blocks_accessed() const override;
    size_t records_output() const override;
    size_t distinct_values(const std::string& fldname) const override;
    std::shared_ptr<record::Schema> schema() const override;

private:
    std::shared_ptr<Plan> p_;
    metadata::IndexInfo ii_;
    Constant val_;
};

} // namespace index

#endif // INDEXSELECTPLAN_HPP
