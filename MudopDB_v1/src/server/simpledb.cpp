#include "server/simpledb.hpp"

namespace server {

SimpleDB::SimpleDB(const std::string& dirname, size_t blocksize, size_t buffsize) {
    fm_ = std::make_shared<file::FileMgr>(dirname, blocksize);
    lm_ = std::make_shared<log::LogMgr>(fm_, LOG_FILE);
    bm_ = std::make_shared<buffer::BufferMgr>(fm_, lm_, buffsize);
}

std::shared_ptr<file::FileMgr> SimpleDB::file_mgr() const { return fm_; }
std::shared_ptr<log::LogMgr> SimpleDB::log_mgr() const { return lm_; }
std::shared_ptr<buffer::BufferMgr> SimpleDB::buffer_mgr() const { return bm_; }

} // namespace server
