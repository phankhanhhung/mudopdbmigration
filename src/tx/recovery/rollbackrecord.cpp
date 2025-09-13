/**
 * RollbackRecord - logs a transaction rollback.
 */

#include "tx/recovery/rollbackrecord.hpp"

namespace tx {

RollbackRecord::RollbackRecord(const file::Page& p) {
    constexpr size_t bytes = 4;
    size_t tpos = bytes;
    txnum_ = static_cast<size_t>(p.get_int(tpos));
}

Op RollbackRecord::op() const {
    return Op::ROLLBACK;
}

std::optional<size_t> RollbackRecord::tx_number() const {
    return txnum_;
}

void RollbackRecord::undo(Transaction& /*tx*/) {
    // No-op
}

size_t RollbackRecord::write_to_log(log::LogMgr& lm, size_t txnum) {
    constexpr size_t bytes = 4;
    std::vector<uint8_t> rec(2 * bytes, 0);
    file::Page p(std::move(rec));
    p.set_int(0, static_cast<int32_t>(Op::ROLLBACK));
    p.set_int(bytes, static_cast<int32_t>(txnum));
    return lm.append(p.contents());
}

} // namespace tx
