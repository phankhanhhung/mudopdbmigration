#ifndef LOGRECORD_HPP
#define LOGRECORD_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace tx {

// Forward declaration
class Transaction;

enum class Op : int32_t {
    CHECKPOINT = 0,
    START = 1,
    COMMIT = 2,
    ROLLBACK = 3,
    SETINT = 4,
    SETSTRING = 5
};

class LogRecord {
public:
    virtual ~LogRecord() = default;

    virtual Op op() const = 0;
    virtual std::optional<size_t> tx_number() const = 0;
    virtual void undo(Transaction& tx) = 0;
};

std::unique_ptr<LogRecord> create_log_record(std::vector<uint8_t> bytes);

} // namespace tx

#endif // LOGRECORD_HPP
