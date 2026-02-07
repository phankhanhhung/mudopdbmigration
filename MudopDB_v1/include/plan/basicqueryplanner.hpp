#ifndef BASICQUERYPLANNER_HPP
#define BASICQUERYPLANNER_HPP

#include "plan/queryplanner.hpp"
#include <memory>

namespace metadata { class MetadataMgr; }

class BasicQueryPlanner : public QueryPlanner {
public:
    explicit BasicQueryPlanner(std::shared_ptr<metadata::MetadataMgr> mdm);

    std::shared_ptr<Plan> create_plan(
        const parse::QueryData& data,
        std::shared_ptr<tx::Transaction> tx) override;

private:
    std::shared_ptr<metadata::MetadataMgr> mdm_;
};

#endif // BASICQUERYPLANNER_HPP
