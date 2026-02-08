#include "api/connection.hpp"
#include "api/statement.hpp"
#include "server/simpledb.hpp"
#include "server/tcp_transport.hpp"
#include "server/protocol.hpp"
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
// NetworkConnection
// ============================================================================

NetworkConnection::NetworkConnection(const std::string& host, uint16_t port) {
    int fd = transport::tcp_connect(host, port);
    channel_ = std::make_shared<transport::TcpChannel>(fd);
}

std::unique_ptr<Statement> NetworkConnection::create_statement() {
    return std::make_unique<NetworkStatement>(
        std::shared_ptr<NetworkConnection>(this, [](NetworkConnection*) {})
    );
}

void NetworkConnection::close() {
    protocol::Buffer req;
    req.write_uint8(static_cast<uint8_t>(protocol::MsgType::CONN_CLOSE));
    auto resp = channel_->rpc(req);
    auto status = static_cast<protocol::Status>(resp.read_uint8());
    if (status == protocol::Status::ERROR) {
        throw std::runtime_error(resp.read_string());
    }
}

void NetworkConnection::commit() {
    protocol::Buffer req;
    req.write_uint8(static_cast<uint8_t>(protocol::MsgType::CONN_COMMIT));
    auto resp = channel_->rpc(req);
    auto status = static_cast<protocol::Status>(resp.read_uint8());
    if (status == protocol::Status::ERROR) {
        throw std::runtime_error(resp.read_string());
    }
}

void NetworkConnection::rollback() {
    protocol::Buffer req;
    req.write_uint8(static_cast<uint8_t>(protocol::MsgType::CONN_ROLLBACK));
    auto resp = channel_->rpc(req);
    auto status = static_cast<protocol::Status>(resp.read_uint8());
    if (status == protocol::Status::ERROR) {
        throw std::runtime_error(resp.read_string());
    }
}
