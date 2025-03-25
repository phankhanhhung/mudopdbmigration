#ifndef CHECKPOINTRECORD_HPP
#define CHECKPOINTRECORD_HPP

#include "tx/recovery/logrecord.hpp"
#include "log/logmgr.hpp"

namespace tx {

/**
 * CheckPointRecord logs a checkpoint.
 * Format: [Op::CHECKPOINT (4B)]
 * Has no transaction number and undo is a no-op.
 *
 * Corresponds to CheckPointRecord in Rust (NMDB2/src/tx/recovery/checkpointrecord.rs)
 */
class CheckPointRecord : public LogRecord {
public:
    CheckPointRecord();

    Op op() const override;
    std::optional<size_t> tx_number() const override;
    void undo(Transaction& tx) override;

    static size_t write_to_log(log::LogMgr& lm);
};

} // namespace tx

#endif // CHECKPOINTRECORD_HPP
