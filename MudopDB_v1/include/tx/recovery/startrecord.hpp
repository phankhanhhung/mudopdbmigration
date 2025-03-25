#ifndef STARTRECORD_HPP
#define STARTRECORD_HPP

#include "tx/recovery/logrecord.hpp"
#include "file/page.hpp"
#include "log/logmgr.hpp"

namespace tx {

/**
 * StartRecord logs the start of a transaction.
 * Format: [Op::START (4B)][txnum (4B)]
 *
 * Corresponds to StartRecord in Rust (NMDB2/src/tx/recovery/startrecord.rs)
 */
class StartRecord : public LogRecord {
public:
    explicit StartRecord(const file::Page& p);

    Op op() const override;
    std::optional<size_t> tx_number() const override;
    void undo(Transaction& tx) override;

    static size_t write_to_log(log::LogMgr& lm, size_t txnum);

private:
    size_t txnum_;
};

} // namespace tx

#endif // STARTRECORD_HPP
