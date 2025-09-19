/**
 * Transaction implementation.
 * Coordinates concurrency control, recovery logging, and buffer management.
 * Provides get/set operations on blocks with automatic locking and WAL logging.
 */

#include "tx/transaction.hpp"
#include "tx/recovery/logrecord.hpp"
#include "tx/recovery/rollbackrecord.hpp"
#include "tx/recovery/checkpointrecord.hpp"
#include <iostream>
#include <stdexcept>

namespace tx {

static std::atomic<size_t> NEXT_TX_NUM{0};

size_t Transaction::next_tx_num() {
    NEXT_TX_NUM.fetch_add(1, std::memory_order_seq_cst);
    return NEXT_TX_NUM.load(std::memory_order_seq_cst);
}

Transaction::Transaction(std::shared_ptr<file::FileMgr> fm,
                         std::shared_ptr<log::LogMgr> lm,
                         std::shared_ptr<buffer::BufferMgr> bm)
    : recovery_mgr_(next_tx_num(), lm, bm),
      concur_mgr_(),
      bm_(bm),
      fm_(fm),
      txnum_(NEXT_TX_NUM.load(std::memory_order_seq_cst)),
      mybuffers_(bm),
      lm_(lm) {}

void Transaction::commit() {
    recovery_mgr_.commit();
    std::cout << "transaction " << txnum_ << " committed" << std::endl;
    concur_mgr_.release();
    mybuffers_.unpin_all();
}

void Transaction::rollback() {
    do_rollback();
    bm_->flush_all(txnum_);
    size_t lsn = RollbackRecord::write_to_log(*lm_, txnum_);
    lm_->flush(lsn);
    std::cout << "transaction " << txnum_ << " rolled back" << std::endl;
    concur_mgr_.release();
    mybuffers_.unpin_all();
}

void Transaction::recover() {
    bm_->flush_all(txnum_);
    do_recover();
    bm_->flush_all(txnum_);
    size_t lsn = CheckPointRecord::write_to_log(*lm_);
    lm_->flush(lsn);
}

void Transaction::pin(const file::BlockId& blk) {
    mybuffers_.pin(blk);
}

void Transaction::unpin(const file::BlockId& blk) {
    mybuffers_.unpin(blk);
}

int32_t Transaction::get_int(const file::BlockId& blk, size_t offset) {
    concur_mgr_.s_lock(blk);
    auto idx = mybuffers_.get_index(blk);
    if (idx.has_value()) {
        auto& buff = bm_->buffer(idx.value());
        return buff.contents().get_int(offset);
    }
    throw std::runtime_error("Transaction::get_int: block not pinned");
}

std::string Transaction::get_string(const file::BlockId& blk, size_t offset) {
    concur_mgr_.s_lock(blk);
    auto idx = mybuffers_.get_index(blk);
    if (idx.has_value()) {
        auto& buff = bm_->buffer(idx.value());
        return buff.contents().get_string(offset);
    }
    throw std::runtime_error("Transaction::get_string: block not pinned");
}

void Transaction::set_int(const file::BlockId& blk, size_t offset,
                           int32_t val, bool ok_to_log) {
    concur_mgr_.x_lock(blk);
    auto idx = mybuffers_.get_index(blk);
    if (idx.has_value()) {
        auto& buff = bm_->buffer(idx.value());
        std::optional<size_t> lsn;
        if (ok_to_log) {
            lsn = recovery_mgr_.set_int(buff, offset, val);
        }
        buff.contents().set_int(offset, val);
        buff.set_modified(txnum_, lsn);
        return;
    }
    throw std::runtime_error("Transaction::set_int: block not pinned");
}

void Transaction::set_string(const file::BlockId& blk, size_t offset,
                              const std::string& val, bool ok_to_log) {
    concur_mgr_.x_lock(blk);
    auto idx = mybuffers_.get_index(blk);
    if (idx.has_value()) {
        auto& buff = bm_->buffer(idx.value());
        std::optional<size_t> lsn;
        if (ok_to_log) {
            lsn = recovery_mgr_.set_string(buff, offset, val);
        }
        buff.contents().set_string(offset, val);
        buff.set_modified(txnum_, lsn);
        return;
    }
    throw std::runtime_error("Transaction::set_string: block not pinned");
}

size_t Transaction::size(const std::string& filename) {
    file::BlockId dummyblk(filename, END_OF_FILE);
    concur_mgr_.s_lock(dummyblk);
    return fm_->length(filename);
}

file::BlockId Transaction::append(const std::string& filename) {
    file::BlockId dummyblk(filename, END_OF_FILE);
    concur_mgr_.x_lock(dummyblk);
    return fm_->append(filename);
}

size_t Transaction::block_size() const {
    return fm_->block_size();
}

size_t Transaction::available_buffs() const {
    return bm_->available();
}

size_t Transaction::tx_num() const {
    return txnum_;
}

void Transaction::do_rollback() {
    std::vector<std::unique_ptr<LogRecord>> recs;
    auto iter = lm_->iterator();
    while (iter->has_next()) {
        auto bytes = iter->next();
        auto rec = create_log_record(std::move(bytes));
        auto txnum = rec->tx_number();
        if (txnum.has_value() && txnum.value() == txnum_) {
            if (rec->op() == Op::START) {
                break;
            }
            recs.push_back(std::move(rec));
        }
    }

    for (auto& rec : recs) {
        rec->undo(*this);
    }
}

void Transaction::do_recover() {
    std::vector<size_t> finished_txs;
    std::vector<std::unique_ptr<LogRecord>> recs;
    auto iter = lm_->iterator();
    while (iter->has_next()) {
        auto bytes = iter->next();
        auto rec = create_log_record(std::move(bytes));
        if (rec->op() == Op::CHECKPOINT) {
            break;
        }
        auto tx_number = rec->tx_number();
        if (tx_number.has_value()) {
            if (rec->op() == Op::COMMIT || rec->op() == Op::ROLLBACK) {
                finished_txs.push_back(tx_number.value());
            } else if (std::find(finished_txs.begin(), finished_txs.end(),
                                  tx_number.value()) == finished_txs.end()) {
                recs.push_back(std::move(rec));
            }
        }
    }
    for (auto& rec : recs) {
        rec->undo(*this);
    }
}

} // namespace tx
