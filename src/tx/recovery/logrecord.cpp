/**
 * LogRecord factory implementation.
 * Deserializes raw log bytes into typed LogRecord objects (Start, Commit,
 * Rollback, SetInt, SetString, Checkpoint) based on the operation tag.
 */

#include "tx/recovery/logrecord.hpp"
#include "tx/recovery/checkpointrecord.hpp"
#include "tx/recovery/startrecord.hpp"
#include "tx/recovery/commitrecord.hpp"
#include "tx/recovery/rollbackrecord.hpp"
#include "tx/recovery/setintrecord.hpp"
#include "tx/recovery/setstringrecord.hpp"
#include "file/page.hpp"
#include <stdexcept>

namespace tx {

std::unique_ptr<LogRecord> create_log_record(std::vector<uint8_t> bytes) {
    file::Page p(std::move(bytes));
    int32_t type = p.get_int(0);

    if (type == static_cast<int32_t>(Op::CHECKPOINT)) {
        return std::make_unique<CheckPointRecord>();
    } else if (type == static_cast<int32_t>(Op::START)) {
        return std::make_unique<StartRecord>(p);
    } else if (type == static_cast<int32_t>(Op::COMMIT)) {
        return std::make_unique<CommitRecord>(p);
    } else if (type == static_cast<int32_t>(Op::ROLLBACK)) {
        return std::make_unique<RollbackRecord>(p);
    } else if (type == static_cast<int32_t>(Op::SETINT)) {
        return std::make_unique<SetIntRecord>(p);
    } else if (type == static_cast<int32_t>(Op::SETSTRING)) {
        return std::make_unique<SetStringRecord>(p);
    }

    throw std::runtime_error("Unknown log record type");
}

} // namespace tx
