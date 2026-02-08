#include "server/simpledb.hpp"
#include "file/memfilemgr.hpp"
#include "opt/heuristicqueryplanner.hpp"
#include "index/planner/indexupdateplanner.hpp"
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

    auto qp = std::make_unique<opt::HeuristicQueryPlanner>(mdm_);
    auto up = std::make_unique<index::IndexUpdatePlanner>(mdm_);
    planner_ = std::make_shared<Planner>(std::move(qp), std::move(up));

    tx->commit();
}

SimpleDB SimpleDB::in_memory() {
    auto fm = std::make_shared<file::MemFileMgr>(BLOCK_SIZE);
    auto lm = std::make_shared<log::LogMgr>(fm, LOG_FILE);
    auto bm = std::make_shared<buffer::BufferMgr>(fm, lm, BUFFER_SIZE);

    SimpleDB db(fm, lm, bm);

    auto tx = db.new_tx();
    db.mdm_ = std::make_shared<metadata::MetadataMgr>(true, tx);

    auto qp = std::make_unique<opt::HeuristicQueryPlanner>(db.mdm_);
    auto up = std::make_unique<::index::IndexUpdatePlanner>(db.mdm_);
    db.planner_ = std::make_shared<Planner>(std::move(qp), std::move(up));

    tx->commit();
    return db;
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
