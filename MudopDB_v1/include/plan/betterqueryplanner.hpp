#ifndef BETTERQUERYPLANNER_HPP
#define BETTERQUERYPLANNER_HPP

#include "plan/queryplanner.hpp"
#include <memory>

namespace metadata { class MetadataMgr; }

/**
 * BetterQueryPlanner evaluates both join orders for each product
 * and picks the one with fewer block accesses.
 *
 * Corresponds to BetterQueryPlanner in Rust (NMDB2/src/plan/betterqueryplanner.rs)
 */
class BetterQueryPlanner : public QueryPlanner {
public:
    explicit BetterQueryPlanner(std::shared_ptr<metadata::MetadataMgr> mdm);

    std::shared_ptr<Plan> create_plan(
        const parse::QueryData& data,
        std::shared_ptr<tx::Transaction> tx) override;

private:
    std::shared_ptr<metadata::MetadataMgr> mdm_;
};

#endif // BETTERQUERYPLANNER_HPP
