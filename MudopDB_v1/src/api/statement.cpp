#include "api/statement.hpp"
#include "api/result_set.hpp"
#include "api/connection.hpp"
#include "server/tcp_transport.hpp"
#include "server/protocol.hpp"
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
// NetworkStatement
// ============================================================================

NetworkStatement::NetworkStatement(std::shared_ptr<NetworkConnection> c)
    : conn_(std::move(c)) {}

std::unique_ptr<ResultSet> NetworkStatement::execute_query(const std::string& qry) {
    protocol::Buffer req;
    req.write_uint8(static_cast<uint8_t>(protocol::MsgType::STMT_EXEC_QUERY));
    req.write_string(qry);
    auto resp = conn_->channel()->rpc(req);
    auto status = static_cast<protocol::Status>(resp.read_uint8());
    if (status == protocol::Status::ERROR) {
        throw std::runtime_error(resp.read_string());
    }
    uint64_t id = resp.read_uint64();
    return std::make_unique<NetworkResultSet>(conn_, id);
}

size_t NetworkStatement::execute_update(const std::string& cmd) {
    protocol::Buffer req;
    req.write_uint8(static_cast<uint8_t>(protocol::MsgType::STMT_EXEC_UPDATE));
    req.write_string(cmd);
    auto resp = conn_->channel()->rpc(req);
    auto status = static_cast<protocol::Status>(resp.read_uint8());
    if (status == protocol::Status::ERROR) {
        throw std::runtime_error(resp.read_string());
    }
    return static_cast<size_t>(resp.read_uint64());
}
