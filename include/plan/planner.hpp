#ifndef PLANNER_HPP
#define PLANNER_HPP

#include "plan/queryplanner.hpp"
#include "plan/updateplanner.hpp"
#include <memory>
#include <string>

/**
 * Main planner that coordinates query and update planners.
 *
 * Corresponds to Planner in Rust (NMDB2/src/plan/planner.rs)
 */
class Planner {
public:
    Planner(std::unique_ptr<QueryPlanner> qplanner,
            std::unique_ptr<UpdatePlanner> uplanner);

    std::shared_ptr<Plan> create_query_plan(
        const std::string& qry,
        std::shared_ptr<tx::Transaction> tx);

    size_t execute_update(
        const std::string& cmd,
        std::shared_ptr<tx::Transaction> tx);

private:
    std::unique_ptr<QueryPlanner> qplanner_;
    std::unique_ptr<UpdatePlanner> uplanner_;
};

#endif // PLANNER_HPP
