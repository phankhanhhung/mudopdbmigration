#ifndef TABLEPLANNER_HPP
#define TABLEPLANNER_HPP

#include "plan/plan.hpp"
#include "plan/tableplan.hpp"
#include "query/predicate.hpp"
#include "metadata/indexinfo.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace metadata { class MetadataMgr; }
namespace tx { class Transaction; }

namespace opt {

/**
 * Plans for a single table within a query.
 * Uses indexes when available for select and join.
 *
 * Corresponds to TablePlanner in Rust (NMDB2/src/opt/tableplanner.rs)
 */
class TablePlanner {
public:
    TablePlanner(const std::string& tblname,
                 const Predicate& mypred,
                 std::shared_ptr<tx::Transaction> tx,
                 std::shared_ptr<metadata::MetadataMgr> mdm);

    std::shared_ptr<Plan> make_select_plan() const;
    std::shared_ptr<Plan> make_join_plan(const std::shared_ptr<Plan>& current) const;
    std::shared_ptr<Plan> make_product_plan(const std::shared_ptr<Plan>& current) const;

private:
    std::shared_ptr<Plan> make_index_select() const;
    std::shared_ptr<Plan> make_index_join(const std::shared_ptr<Plan>& current,
                                           std::shared_ptr<record::Schema> currsch) const;
    std::shared_ptr<Plan> make_product_join(const std::shared_ptr<Plan>& current,
                                             std::shared_ptr<record::Schema> currsch) const;
    std::shared_ptr<Plan> add_select_pred(std::shared_ptr<Plan> p) const;
    std::shared_ptr<Plan> add_join_pred(std::shared_ptr<Plan> p,
                                         std::shared_ptr<record::Schema> currsch) const;

    std::shared_ptr<::TablePlan> myplan_;
    Predicate mypred_;
    std::shared_ptr<record::Schema> myschema_;
    std::unordered_map<std::string, metadata::IndexInfo> indexes_;
    std::shared_ptr<tx::Transaction> tx_;
};

} // namespace opt

#endif // TABLEPLANNER_HPP
