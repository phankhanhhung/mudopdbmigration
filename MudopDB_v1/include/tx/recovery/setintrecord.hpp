#ifndef SETINTRECORD_HPP
#define SETINTRECORD_HPP

#include "tx/recovery/logrecord.hpp"
#include "file/page.hpp"
#include "file/blockid.hpp"
#include "log/logmgr.hpp"

namespace tx {

/**
 * SetIntRecord logs an integer field modification.
 * Format: [Op::SETINT (4B)][txnum (4B)][filename (var)][blknum (4B)][offset (4B)][val (4B)]
 * val stores the OLD value for undo.
 *
 * Corresponds to SetIntRecord in Rust (NMDB2/src/tx/recovery/setintrecord.rs)
 */
class SetIntRecord : public LogRecord {
public:
    explicit SetIntRecord(const file::Page& p);

    Op op() const override;
    std::optional<size_t> tx_number() const override;
    void undo(Transaction& tx) override;

    static size_t write_to_log(log::LogMgr& lm, size_t txnum,
                               const file::BlockId& blk, size_t offset, int32_t val);

private:
    size_t txnum_;
    size_t offset_;
    int32_t val_;
    file::BlockId blk_;
};

} // namespace tx

#endif // SETINTRECORD_HPP
