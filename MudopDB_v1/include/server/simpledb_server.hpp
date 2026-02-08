#ifndef SIMPLEDB_SERVER_HPP
#define SIMPLEDB_SERVER_HPP

#include "server/simpledb.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <cstdint>

class EmbeddedConnection;
class EmbeddedResultSet;

namespace server {

/**
 * SimpleDB server that wraps the embedded API for remote access.
 *
 * Corresponds to the Remote* types in Rust:
 *   - RemoteConnection (NMDB2/src/api/network/remoteconnection.rs)
 *   - RemoteStatement (NMDB2/src/api/network/remotestatement.rs)
 *   - RemoteResultSet (NMDB2/src/api/network/remoteresultset.rs)
 *   - RemoteMetaData (NMDB2/src/api/network/remotemetadata.rs)
 *
 * In the Rust version these are gRPC service implementations.
 * In C++ this provides the same logical API, ready for a future
 * gRPC or TCP transport layer.
 */
class SimpleDBServer {
public:
    explicit SimpleDBServer(const std::string& dbname);
    ~SimpleDBServer();

    // Connection operations
    void commit();
    void rollback();
    void close_connection();

    // Statement operations
    uint64_t execute_query(const std::string& query);
    uint64_t execute_update(const std::string& command);

    // ResultSet operations (by id)
    bool next(uint64_t id);
    int32_t get_int(uint64_t id, const std::string& fldname);
    std::string get_string(uint64_t id, const std::string& fldname);
    void close_result_set(uint64_t id);

    // MetaData operations (by id)
    size_t get_column_count(uint64_t id);
    std::string get_column_name(uint64_t id, size_t column);
    int32_t get_column_type(uint64_t id, size_t column);
    size_t get_column_display_size(uint64_t id, size_t column);

private:
    SimpleDB db_;
    std::shared_ptr<EmbeddedConnection> conn_;
    std::mutex mutex_;

    std::unordered_map<uint64_t, std::unique_ptr<class ResultSetHolder>> result_sets_;
    uint64_t next_rs_id_ = 0;
};

} // namespace server

#endif // SIMPLEDB_SERVER_HPP
