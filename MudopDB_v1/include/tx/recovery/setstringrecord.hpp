#ifndef SETSTRINGRECORD_HPP
#define SETSTRINGRECORD_HPP

#include "tx/recovery/logrecord.hpp"
#include "file/page.hpp"
#include "file/blockid.hpp"
#include "log/logmgr.hpp"
#include <string>

namespace tx {

/**
 * SetStringRecord logs a string field modification.
 * Format: [Op::SETSTRING (4B)][txnum (4B)][filename (var)][blknum (4B)][offset (4B)][val (var)]
 * val stores the OLD value for undo.
 *
 * Corresponds to SetStringRecord in Rust (NMDB2/src/tx/recovery/setstringrecord.rs)
 */
class SetStringRecord : public LogRecord {
public:
    explicit SetStringRecord(const file::Page& p);

    Op op() const override;
    std::optional<size_t> tx_number() const override;
    void undo(Transaction& tx) override;

    static size_t write_to_log(log::LogMgr& lm, size_t txnum,
                               const file::BlockId& blk, size_t offset,
                               const std::string& val);

private:
    size_t txnum_;
    size_t offset_;
    std::string val_;
    file::BlockId blk_;
};

} // namespace tx

#endif // SETSTRINGRECORD_HPP
