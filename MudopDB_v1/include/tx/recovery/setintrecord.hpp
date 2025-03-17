#ifndef SETINTRECORD_HPP
#define SETINTRECORD_HPP

#include "tx/recovery/logrecord.hpp"
#include "file/page.hpp"
#include "file/blockid.hpp"
#include "log/logmgr.hpp"

namespace tx {

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
