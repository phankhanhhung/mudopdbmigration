#ifndef STATEMENT_HPP
#define STATEMENT_HPP

#include <iostream>
#include <string>
#include <memory>

class ResultSet;
class EmbeddedConnection;
class NetworkConnection;

class Statement {
public:
  virtual ~Statement() = default;
  virtual std::unique_ptr<ResultSet> execute_query(std::string qry) = 0;
  virtual size_t execute_update(std::string cmd) = 0;
};

class EmbeddedStatement : public Statement {
public:
  explicit EmbeddedStatement(std::shared_ptr<EmbeddedConnection> conn);
  std::unique_ptr<ResultSet> execute_query(std::string qry) override;
  size_t execute_update(std::string cmd) override;
private:
  std::shared_ptr<EmbeddedConnection> conn;
};
class NetworkStatement  : public Statement {
public:
  explicit NetworkStatement(std::shared_ptr<NetworkConnection> conn);
  std::unique_ptr<ResultSet> execute_query(std::string qry) override;
  size_t execute_update(std::string cmd) override;
private:
  std::shared_ptr<NetworkConnection> conn;
};

#endif // STATEMENT_HPP