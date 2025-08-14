#include "api/statement.hpp"
#include "api/result_set.hpp"
#include "api/connection.hpp"
#include <memory>
#include <utility>

// EmbeddedStatement
EmbeddedStatement::EmbeddedStatement(std::shared_ptr<EmbeddedConnection> c)
	: conn(std::move(c)) {}

std::unique_ptr<ResultSet> EmbeddedStatement::execute_query(std::string /*qry*/) {
	// Return a dummy ResultSet that yields no rows
	return std::unique_ptr<ResultSet>(new EmbeddedResultSet(std::shared_ptr<Plan>{}, conn));
}

size_t EmbeddedStatement::execute_update(std::string /*cmd*/) {
	// Pretend one row was affected
	return 1;
}

// NetworkStatement
NetworkStatement::NetworkStatement(std::shared_ptr<NetworkConnection> c)
	: conn(std::move(c)) {}

std::unique_ptr<ResultSet> NetworkStatement::execute_query(std::string /*qry*/) {
	return std::unique_ptr<ResultSet>(new NetworkResultSet(conn, 0));
}

size_t NetworkStatement::execute_update(std::string /*cmd*/) {
	return 1;
}

