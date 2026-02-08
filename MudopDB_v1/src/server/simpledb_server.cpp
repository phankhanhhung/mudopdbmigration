#include "server/simpledb_server.hpp"
#include "api/connection.hpp"
#include "api/statement.hpp"
#include "api/result_set.hpp"
#include "api/metadata.hpp"
#include "plan/plan.hpp"
#include "query/scan.hpp"
#include <stdexcept>

namespace server {

// Internal holder for a ResultSet and its Metadata
class ResultSetHolder {
public:
    ResultSetHolder(std::unique_ptr<ResultSet> rs)
        : rs_(std::move(rs)) {}

    ResultSet* get() { return rs_.get(); }

private:
    std::unique_ptr<ResultSet> rs_;
};

SimpleDBServer::~SimpleDBServer() = default;

SimpleDBServer::SimpleDBServer(const std::string& dbname)
    : db_(dbname) {
    auto db_ptr = std::make_shared<SimpleDB>(dbname);
    conn_ = std::make_shared<EmbeddedConnection>(db_ptr);
}

void SimpleDBServer::commit() {
    std::lock_guard<std::mutex> lock(mutex_);
    conn_->commit();
}

void SimpleDBServer::rollback() {
    std::lock_guard<std::mutex> lock(mutex_);
    conn_->rollback();
}

void SimpleDBServer::close_connection() {
    std::lock_guard<std::mutex> lock(mutex_);
    conn_->close();
}

uint64_t SimpleDBServer::execute_query(const std::string& query) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto stmt = conn_->create_statement();
    auto rs = stmt->execute_query(query);
    uint64_t id = next_rs_id_++;
    result_sets_[id] = std::make_unique<ResultSetHolder>(std::move(rs));
    return id;
}

uint64_t SimpleDBServer::execute_update(const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto stmt = conn_->create_statement();
    size_t count = stmt->execute_update(command);
    return static_cast<uint64_t>(count);
}

bool SimpleDBServer::next(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = result_sets_.find(id);
    if (it == result_sets_.end()) {
        throw std::runtime_error("SimpleDBServer: invalid result set id");
    }
    return it->second->get()->next();
}

int32_t SimpleDBServer::get_int(uint64_t id, const std::string& fldname) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = result_sets_.find(id);
    if (it == result_sets_.end()) {
        throw std::runtime_error("SimpleDBServer: invalid result set id");
    }
    return it->second->get()->get_int(fldname);
}

std::string SimpleDBServer::get_string(uint64_t id, const std::string& fldname) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = result_sets_.find(id);
    if (it == result_sets_.end()) {
        throw std::runtime_error("SimpleDBServer: invalid result set id");
    }
    return it->second->get()->get_string(fldname);
}

void SimpleDBServer::close_result_set(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = result_sets_.find(id);
    if (it != result_sets_.end()) {
        it->second->get()->close();
        result_sets_.erase(it);
    }
}

size_t SimpleDBServer::get_column_count(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = result_sets_.find(id);
    if (it == result_sets_.end()) {
        throw std::runtime_error("SimpleDBServer: invalid result set id");
    }
    auto md = it->second->get()->get_meta_data();
    return md->get_column_count();
}

std::string SimpleDBServer::get_column_name(uint64_t id, size_t column) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = result_sets_.find(id);
    if (it == result_sets_.end()) {
        throw std::runtime_error("SimpleDBServer: invalid result set id");
    }
    auto md = it->second->get()->get_meta_data();
    return md->get_column_name(column);
}

int32_t SimpleDBServer::get_column_type(uint64_t id, size_t column) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = result_sets_.find(id);
    if (it == result_sets_.end()) {
        throw std::runtime_error("SimpleDBServer: invalid result set id");
    }
    auto md = it->second->get()->get_meta_data();
    return static_cast<int32_t>(md->get_column_type(column));
}

size_t SimpleDBServer::get_column_display_size(uint64_t id, size_t column) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = result_sets_.find(id);
    if (it == result_sets_.end()) {
        throw std::runtime_error("SimpleDBServer: invalid result set id");
    }
    auto md = it->second->get()->get_meta_data();
    return md->get_column_display_size(column);
}

} // namespace server
