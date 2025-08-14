#include "server/simpledb.hpp"
#include "plan/basicqueryplanner.hpp"
#include "plan/basicupdateplanner.hpp"
#include <iostream>

namespace server {

SimpleDB::SimpleDB(std::shared_ptr<file::FileMgr> fm,
                   std::shared_ptr<log::LogMgr> lm,
                   std::shared_ptr<buffer::BufferMgr> bm)
    : fm_(fm), lm_(lm), bm_(bm) {}

SimpleDB SimpleDB::with_params(const std::string& dirname,
                                size_t blocksize,
                                size_t buffsize) {
    auto fm = std::make_shared<file::FileMgr>(dirname, blocksize);
    auto lm = std::make_shared<log::LogMgr>(fm, LOG_FILE);
    auto bm = std::make_shared<buffer::BufferMgr>(fm, lm, buffsize);
    return SimpleDB(fm, lm, bm);
}

SimpleDB::SimpleDB(const std::string& dirname) : SimpleDB(nullptr, nullptr, nullptr) {
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

    auto qp = std::make_unique<BasicQueryPlanner>(mdm_);
    auto up = std::make_unique<BasicUpdatePlanner>(mdm_);
    planner_ = std::make_shared<Planner>(std::move(qp), std::move(up));

    tx->commit();
}

std::shared_ptr<tx::Transaction> SimpleDB::new_tx() {
    return std::make_shared<tx::Transaction>(fm_, lm_, bm_);
}

std::shared_ptr<metadata::MetadataMgr> SimpleDB::md_mgr() const {
    return mdm_;
}

std::shared_ptr<file::FileMgr> SimpleDB::file_mgr() const {
    return fm_;
}

std::shared_ptr<log::LogMgr> SimpleDB::log_mgr() const {
    return lm_;
}

std::shared_ptr<buffer::BufferMgr> SimpleDB::buffer_mgr() const {
    return bm_;
}

std::shared_ptr<Planner> SimpleDB::planner() const {
    return planner_;
}

} // namespace server
