#ifndef SIMPLEDB_HPP
#define SIMPLEDB_HPP

#include "file/filemgr.hpp"
#include "log/logmgr.hpp"
#include "buffer/buffermgr.hpp"
#include <memory>
#include <string>

namespace server {

class SimpleDB {
public:
    SimpleDB(const std::string& dirname, size_t blocksize, size_t buffsize);

    std::shared_ptr<file::FileMgr> file_mgr() const;
    std::shared_ptr<log::LogMgr> log_mgr() const;
    std::shared_ptr<buffer::BufferMgr> buffer_mgr() const;

private:
    static constexpr const char* LOG_FILE = "simpledb.log";

    std::shared_ptr<file::FileMgr> fm_;
    std::shared_ptr<log::LogMgr> lm_;
    std::shared_ptr<buffer::BufferMgr> bm_;
};

} // namespace server

#endif // SIMPLEDB_HPP
