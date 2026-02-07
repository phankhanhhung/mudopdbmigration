#include "api/connection.hpp"
#include "api/statement.hpp"
#include "server/simpledb.hpp"
#include "tx/transaction.hpp"
#include "plan/planner.hpp"
#include <memory>
#include <stdexcept>

// ============================================================================
// EmbeddedConnection
// ============================================================================

EmbeddedConnection::EmbeddedConnection(std::shared_ptr<server::SimpleDB> db)
    : db_(db), current_tx_(nullptr), planner_(nullptr) {
    if (db_) {
        current_tx_ = db_->new_tx();
        planner_ = db_->planner();
    }
}

std::unique_ptr<Statement> EmbeddedConnection::create_statement() {
    return std::make_unique<EmbeddedStatement>(
        std::shared_ptr<EmbeddedConnection>(this, [](EmbeddedConnection*) {})
    );
}

void EmbeddedConnection::close() {
    if (current_tx_) {
        current_tx_->commit();
    }
}

void EmbeddedConnection::commit() {
    if (current_tx_) {
        current_tx_->commit();
        current_tx_ = db_->new_tx();
    }
}

void EmbeddedConnection::rollback() {
    if (current_tx_) {
        current_tx_->rollback();
        current_tx_ = db_->new_tx();
    }
}

std::shared_ptr<tx::Transaction> EmbeddedConnection::get_transaction() {
    return current_tx_;
}

std::shared_ptr<Planner> EmbeddedConnection::planner() {
    return planner_;
}

// ============================================================================
// NetworkConnection (stub)
// ============================================================================

std::unique_ptr<Statement> NetworkConnection::create_statement() {
    return std::make_unique<NetworkStatement>(
        std::shared_ptr<NetworkConnection>(this, [](NetworkConnection*) {})
    );
}

void NetworkConnection::close() {}
void NetworkConnection::commit() {}
void NetworkConnection::rollback() {}
