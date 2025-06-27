#include "server/simpledb.hpp"
#include <iostream>

namespace server {

SimpleDB::SimpleDB(const std::string& dirname, size_t blocksize, size_t buffsize) {
    fm_ = std::make_shared<file::FileMgr>(dirname, blocksize);
    lm_ = std::make_shared<log::LogMgr>(fm_, LOG_FILE);
    bm_ = std::make_shared<buffer::BufferMgr>(fm_, lm_, buffsize);
}

SimpleDB::SimpleDB(const std::string& dirname) {
    fm_ = std::make_shared<file::FileMgr>(dirname, BLOCK_SIZE);
    lm_ = std::make_shared<log::LogMgr>(fm_, LOG_FILE);
    bm_ = std::make_shared<buffer::BufferMgr>(fm_, lm_, BUFFER_SIZE);

    auto tx = new_tx();
    bool isnew = fm_->is_new();

    if (isnew) {
        std::cout << "creating new database" << std::endl;
    } else {
        std::cout << "recovering existing database" << std::endl;
        tx->recover();
    }

    mdm_ = std::make_shared<metadata::MetadataMgr>(isnew, tx);
    tx->commit();
}

std::shared_ptr<tx::Transaction> SimpleDB::new_tx() {
    return std::make_shared<tx::Transaction>(fm_, lm_, bm_);
}

std::shared_ptr<metadata::MetadataMgr> SimpleDB::md_mgr() const { return mdm_; }
std::shared_ptr<file::FileMgr> SimpleDB::file_mgr() const { return fm_; }
std::shared_ptr<log::LogMgr> SimpleDB::log_mgr() const { return lm_; }
std::shared_ptr<buffer::BufferMgr> SimpleDB::buffer_mgr() const { return bm_; }

} // namespace server
