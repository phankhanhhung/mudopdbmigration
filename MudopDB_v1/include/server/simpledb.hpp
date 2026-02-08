#ifndef SIMPLEDB_HPP
#define SIMPLEDB_HPP

#include "file/filemgr.hpp"
#include "log/logmgr.hpp"
#include "buffer/buffermgr.hpp"
#include "metadata/metadatamgr.hpp"
#include "tx/transaction.hpp"
#include "plan/planner.hpp"
#include <memory>
#include <string>
#include <optional>

namespace server {

/**
 * SimpleDB is the top-level database server.
 * Wires together FileMgr, LogMgr, BufferMgr, and MetadataMgr.
 *
 * Corresponds to SimpleDB in Rust (NMDB2/src/server/simpledb.rs)
 *
 */
class SimpleDB {
public:
    /**
     * Creates a SimpleDB instance with custom parameters.
     * Does NOT initialize metadata or run recovery.
     */
    static SimpleDB with_params(const std::string& dirname,
                                size_t blocksize,
                                size_t buffsize);

    /**
     * Creates a fully initialized SimpleDB instance.
     * Runs recovery if existing database, initializes metadata if new.
     */
    SimpleDB(const std::string& dirname);

    /**
     * Creates a fully initialized in-memory SimpleDB instance.
     * No disk I/O - all data lives in memory only.
     */
    static SimpleDB in_memory();

    std::shared_ptr<tx::Transaction> new_tx();

    std::shared_ptr<metadata::MetadataMgr> md_mgr() const;
    std::shared_ptr<file::FileMgr> file_mgr() const;
    std::shared_ptr<log::LogMgr> log_mgr() const;
    std::shared_ptr<buffer::BufferMgr> buffer_mgr() const;
    std::shared_ptr<Planner> planner() const;

private:
    static constexpr size_t BLOCK_SIZE = 400;
    static constexpr size_t BUFFER_SIZE = 8;
    static constexpr const char* LOG_FILE = "simpledb.log";

    SimpleDB(std::shared_ptr<file::FileMgr> fm,
             std::shared_ptr<log::LogMgr> lm,
             std::shared_ptr<buffer::BufferMgr> bm);

    std::shared_ptr<file::FileMgr> fm_;
    std::shared_ptr<log::LogMgr> lm_;
    std::shared_ptr<buffer::BufferMgr> bm_;
    std::shared_ptr<metadata::MetadataMgr> mdm_;
    std::shared_ptr<Planner> planner_;
};

} // namespace server

#endif // SIMPLEDB_HPP
