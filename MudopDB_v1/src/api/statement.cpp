#include "api/statement.hpp"
#include "api/result_set.hpp"
#include "api/connection.hpp"
#include "plan/planner.hpp"
#include "plan/plan.hpp"
#include "query/scan.hpp"
#include <memory>
#include <stdexcept>
#include <utility>

// ============================================================================
// EmbeddedStatement
// ============================================================================

EmbeddedStatement::EmbeddedStatement(std::shared_ptr<EmbeddedConnection> c)
    : conn_(std::move(c)) {}

std::unique_ptr<ResultSet> EmbeddedStatement::execute_query(const std::string& qry) {
    auto tx = conn_->get_transaction();
    auto p = conn_->planner();
    if (p) {
        try {
            auto plan = p->create_query_plan(qry, tx);
            if (plan) {
                return std::make_unique<EmbeddedResultSet>(plan, conn_);
            }
        } catch (const std::exception& e) {
            conn_->rollback();
            throw std::runtime_error(std::string("execute_query: ") + e.what());
        }
    }
    conn_->rollback();
    throw std::runtime_error("EmbeddedStatement: execute_query failed - no planner");
}

size_t EmbeddedStatement::execute_update(const std::string& cmd) {
    auto tx = conn_->get_transaction();
    auto p = conn_->planner();
    if (p) {
        try {
            size_t result = p->execute_update(cmd, tx);
            conn_->commit();
            return result;
        } catch (const std::exception& e) {
            conn_->rollback();
            throw std::runtime_error(std::string("execute_update: ") + e.what());
        }
    }
    conn_->rollback();
    throw std::runtime_error("EmbeddedStatement: execute_update failed - no planner");
}

// ============================================================================
// NetworkStatement (stub)
// ============================================================================

NetworkStatement::NetworkStatement(std::shared_ptr<NetworkConnection> c)
    : conn_(std::move(c)) {}

std::unique_ptr<ResultSet> NetworkStatement::execute_query(const std::string& qry) {
    (void)qry;
    return std::make_unique<NetworkResultSet>(conn_, 0);
}

size_t NetworkStatement::execute_update(const std::string& cmd) {
    (void)cmd;
    return 0;
}
