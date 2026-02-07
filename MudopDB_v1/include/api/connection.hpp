#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <memory>

namespace server { class SimpleDB; }
namespace tx { class Transaction; }
class Planner;
class Statement;

/**
 * Abstract base class for database connections.
 * Corresponds to ConnectionControl trait in Rust (NMDB2/src/api/connection.rs)
 */
class Connection {
public:
    virtual ~Connection() = default;
    virtual std::unique_ptr<Statement> create_statement() = 0;
    virtual void close() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
};

/**
 * Embedded database connection.
 * Corresponds to EmbeddedConnection in Rust (NMDB2/src/api/embedded/embeddedconnection.rs)
 */
class EmbeddedConnection : public Connection {
public:
    explicit EmbeddedConnection(std::shared_ptr<server::SimpleDB> db);
    ~EmbeddedConnection() override = default;

    std::unique_ptr<Statement> create_statement() override;
    void close() override;
    void commit() override;
    void rollback() override;

    std::shared_ptr<tx::Transaction> get_transaction();
    std::shared_ptr<Planner> planner();

private:
    std::shared_ptr<server::SimpleDB> db_;
    std::shared_ptr<tx::Transaction> current_tx_;
    std::shared_ptr<Planner> planner_;
};

/**
 * Network database connection (stub).
 */
class NetworkConnection : public Connection {
public:
    std::unique_ptr<Statement> create_statement() override;
    void close() override;
    void commit() override;
    void rollback() override;
};

#endif // CONNECTION_HPP
