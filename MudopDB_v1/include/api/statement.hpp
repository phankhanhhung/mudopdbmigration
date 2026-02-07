#ifndef STATEMENT_HPP
#define STATEMENT_HPP

#include <memory>
#include <string>

class ResultSet;
class EmbeddedConnection;
class NetworkConnection;

/**
 * Abstract base class for SQL statements.
 * Corresponds to StatementControl trait in Rust (NMDB2/src/api/statement.rs)
 */
class Statement {
public:
    virtual ~Statement() = default;
    virtual std::unique_ptr<ResultSet> execute_query(const std::string& qry) = 0;
    virtual size_t execute_update(const std::string& cmd) = 0;
};

/**
 * Embedded statement executing queries/updates via the Planner.
 * Corresponds to EmbeddedStatement in Rust (NMDB2/src/api/embedded/embeddedstatement.rs)
 */
class EmbeddedStatement : public Statement {
public:
    explicit EmbeddedStatement(std::shared_ptr<EmbeddedConnection> conn);
    std::unique_ptr<ResultSet> execute_query(const std::string& qry) override;
    size_t execute_update(const std::string& cmd) override;

private:
    std::shared_ptr<EmbeddedConnection> conn_;
};

/**
 * Network statement (stub).
 */
class NetworkStatement : public Statement {
public:
    explicit NetworkStatement(std::shared_ptr<NetworkConnection> conn);
    std::unique_ptr<ResultSet> execute_query(const std::string& qry) override;
    size_t execute_update(const std::string& cmd) override;

private:
    std::shared_ptr<NetworkConnection> conn_;
};

#endif // STATEMENT_HPP
