#ifndef SIMPLEDB_HPP
#define SIMPLEDB_HPP

#include "file/filemgr.hpp"
#include "log/logmgr.hpp"
#include "buffer/buffermgr.hpp"
#include "metadata/metadatamgr.hpp"
#include "tx/transaction.hpp"
#include <memory>
#include <string>

namespace server {

class SimpleDB {
public:
    SimpleDB(const std::string& dirname, size_t blocksize, size_t buffsize);
    SimpleDB(const std::string& dirname);

    std::shared_ptr<tx::Transaction> new_tx();
    std::shared_ptr<metadata::MetadataMgr> md_mgr() const;
    std::shared_ptr<file::FileMgr> file_mgr() const;
    std::shared_ptr<log::LogMgr> log_mgr() const;
    std::shared_ptr<buffer::BufferMgr> buffer_mgr() const;

private:
    static constexpr size_t BLOCK_SIZE = 400;
    static constexpr size_t BUFFER_SIZE = 8;
    static constexpr const char* LOG_FILE = "simpledb.log";

    std::shared_ptr<file::FileMgr> fm_;
    std::shared_ptr<log::LogMgr> lm_;
    std::shared_ptr<buffer::BufferMgr> bm_;
    std::shared_ptr<metadata::MetadataMgr> mdm_;
};

} // namespace server

#endif // SIMPLEDB_HPP
