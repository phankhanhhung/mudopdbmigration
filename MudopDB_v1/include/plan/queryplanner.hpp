#ifndef QUERYPLANNER_HPP
#define QUERYPLANNER_HPP

#include <memory>

namespace parse { class QueryData; }
namespace tx { class Transaction; }
class Plan;

class QueryPlanner {
public:
    virtual ~QueryPlanner() = default;
    virtual std::shared_ptr<Plan> create_plan(
        const parse::QueryData& data,
        std::shared_ptr<tx::Transaction> tx) = 0;
};

#endif // QUERYPLANNER_HPP
