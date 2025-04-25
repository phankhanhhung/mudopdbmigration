#ifndef TRANSACTION_HPP
#define TRANSACTION_HPP

#include "file/filemgr.hpp"
#include "file/blockid.hpp"
#include "log/logmgr.hpp"
#include "buffer/buffermgr.hpp"
#include "tx/recovery/recoverymgr.hpp"
#include "tx/concurrency/concurrencymgr.hpp"
#include "tx/bufferlist.hpp"
#include <memory>
#include <string>
#include <atomic>

namespace tx {

/**
 * Transaction provides transaction semantics for the database.
 * Manages concurrency control, recovery, and buffer access.
 *
 * Corresponds to Transaction in Rust (NMDB2/src/tx/transaction.rs)
 */
class Transaction {
public:
    Transaction(std::shared_ptr<file::FileMgr> fm,
                std::shared_ptr<log::LogMgr> lm,
                std::shared_ptr<buffer::BufferMgr> bm);

    void commit();
    void rollback();
    void recover();

    void pin(const file::BlockId& blk);
    void unpin(const file::BlockId& blk);

    int32_t get_int(const file::BlockId& blk, size_t offset);
    std::string get_string(const file::BlockId& blk, size_t offset);
    void set_int(const file::BlockId& blk, size_t offset, int32_t val, bool ok_to_log);
    void set_string(const file::BlockId& blk, size_t offset,
                    const std::string& val, bool ok_to_log);

    size_t size(const std::string& filename);
    file::BlockId append(const std::string& filename);
    size_t block_size() const;
    size_t available_buffs() const;

    size_t tx_num() const;

private:
    static constexpr int32_t END_OF_FILE = -1;

    void do_rollback();
    void do_recover();

    static size_t next_tx_num();

    RecoveryMgr recovery_mgr_;
    ConcurrencyMgr concur_mgr_;
    std::shared_ptr<buffer::BufferMgr> bm_;
    std::shared_ptr<file::FileMgr> fm_;
    size_t txnum_;
    BufferList mybuffers_;
    std::shared_ptr<log::LogMgr> lm_;
};

} // namespace tx

#endif // TRANSACTION_HPP
