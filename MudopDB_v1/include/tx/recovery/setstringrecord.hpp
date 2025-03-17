#ifndef SETSTRINGRECORD_HPP
#define SETSTRINGRECORD_HPP

#include "tx/recovery/logrecord.hpp"
#include "file/page.hpp"
#include "file/blockid.hpp"
#include "log/logmgr.hpp"
#include <string>

namespace tx {

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
