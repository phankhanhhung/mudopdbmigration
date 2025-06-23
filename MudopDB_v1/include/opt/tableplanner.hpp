#ifndef TABLEPLANNER_HPP
#define TABLEPLANNER_HPP

#include "plan/plan.hpp"
#include "plan/tableplan.hpp"
#include "query/predicate.hpp"
#include <memory>
#include <string>

namespace metadata { class MetadataMgr; }
namespace tx { class Transaction; }

namespace opt {

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
    std::shared_ptr<Plan> add_select_pred(std::shared_ptr<Plan> p) const;
    std::shared_ptr<Plan> add_join_pred(std::shared_ptr<Plan> p,
                                         std::shared_ptr<record::Schema> currsch) const;

    std::shared_ptr<::TablePlan> myplan_;
    Predicate mypred_;
    std::shared_ptr<record::Schema> myschema_;
    std::shared_ptr<tx::Transaction> tx_;
};

} // namespace opt

#endif // TABLEPLANNER_HPP
