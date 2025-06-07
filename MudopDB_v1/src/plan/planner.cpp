#include "plan/planner.hpp"
#include "parse/parser.hpp"
#include "parse/querydata.hpp"
#include "parse/insertdata.hpp"
#include "parse/deletedata.hpp"
#include "parse/modifydata.hpp"
#include "parse/createtabledata.hpp"
#include "parse/createviewdata.hpp"
#include "parse/createindexdata.hpp"
#include "tx/transaction.hpp"

Planner::Planner(std::unique_ptr<QueryPlanner> qplanner,
                 std::unique_ptr<UpdatePlanner> uplanner)
    : qplanner_(std::move(qplanner)), uplanner_(std::move(uplanner)) {}

std::shared_ptr<Plan> Planner::create_query_plan(
    const std::string& qry,
    std::shared_ptr<tx::Transaction> tx) {
    parse::Parser parser(qry);
    auto data = parser.query();
    return qplanner_->create_plan(data, tx);
}

size_t Planner::execute_update(
    const std::string& cmd,
    std::shared_ptr<tx::Transaction> tx) {
    parse::Parser parser(cmd);
    auto data = parser.update_cmd();

    return std::visit([&](auto&& obj) -> size_t {
        using T = std::decay_t<decltype(obj)>;
        if constexpr (std::is_same_v<T, parse::InsertData>) {
            return uplanner_->execute_insert(obj, tx);
        } else if constexpr (std::is_same_v<T, parse::DeleteData>) {
            return uplanner_->execute_delete(obj, tx);
        } else if constexpr (std::is_same_v<T, parse::ModifyData>) {
            return uplanner_->execute_modify(obj, tx);
        } else if constexpr (std::is_same_v<T, parse::CreateTableData>) {
            return uplanner_->execute_create_table(obj, tx);
        } else if constexpr (std::is_same_v<T, parse::CreateViewData>) {
            return uplanner_->execute_create_view(obj, tx);
        } else if constexpr (std::is_same_v<T, parse::CreateIndexData>) {
            return uplanner_->execute_create_index(obj, tx);
        }
        return 0;
    }, data);
}
