#ifndef CHECKPOINTRECORD_HPP
#define CHECKPOINTRECORD_HPP

#include "tx/recovery/logrecord.hpp"
#include "log/logmgr.hpp"

namespace tx {

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
