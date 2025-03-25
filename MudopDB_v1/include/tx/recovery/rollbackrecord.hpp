#ifndef ROLLBACKRECORD_HPP
#define ROLLBACKRECORD_HPP

#include "tx/recovery/logrecord.hpp"
#include "file/page.hpp"
#include "log/logmgr.hpp"

namespace tx {

/**
 * RollbackRecord logs the rollback of a transaction.
 * Format: [Op::ROLLBACK (4B)][txnum (4B)]
 *
 * Corresponds to RollbackRecord in Rust (NMDB2/src/tx/recovery/rollbackrecord.rs)
 */
class RollbackRecord : public LogRecord {
public:
    explicit RollbackRecord(const file::Page& p);

    Op op() const override;
    std::optional<size_t> tx_number() const override;
    void undo(Transaction& tx) override;

    static size_t write_to_log(log::LogMgr& lm, size_t txnum);

private:
    size_t txnum_;
};

} // namespace tx

#endif // ROLLBACKRECORD_HPP
