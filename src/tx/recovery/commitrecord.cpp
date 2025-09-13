/**
 * CommitRecord - logs a successful transaction commit.
 */

#include "tx/recovery/commitrecord.hpp"

namespace tx {

CommitRecord::CommitRecord(const file::Page& p) {
    constexpr size_t bytes = 4;
    size_t tpos = bytes;
    txnum_ = static_cast<size_t>(p.get_int(tpos));
}

Op CommitRecord::op() const {
    return Op::COMMIT;
}

std::optional<size_t> CommitRecord::tx_number() const {
    return txnum_;
}

void CommitRecord::undo(Transaction& /*tx*/) {
    // No-op
}

size_t CommitRecord::write_to_log(log::LogMgr& lm, size_t txnum) {
    constexpr size_t bytes = 4;
    std::vector<uint8_t> rec(2 * bytes, 0);
    file::Page p(std::move(rec));
    p.set_int(0, static_cast<int32_t>(Op::COMMIT));
    p.set_int(bytes, static_cast<int32_t>(txnum));
    return lm.append(p.contents());
}

} // namespace tx
