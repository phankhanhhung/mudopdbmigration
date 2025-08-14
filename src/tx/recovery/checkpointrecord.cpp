#include "tx/recovery/checkpointrecord.hpp"

namespace tx {

CheckPointRecord::CheckPointRecord() {}

Op CheckPointRecord::op() const {
    return Op::CHECKPOINT;
}

std::optional<size_t> CheckPointRecord::tx_number() const {
    return std::nullopt;
}

void CheckPointRecord::undo(Transaction& /*tx*/) {
    // No-op
}

size_t CheckPointRecord::write_to_log(log::LogMgr& lm) {
    constexpr size_t bytes = 4;
    std::vector<uint8_t> rec(bytes, 0);
    file::Page p(std::move(rec));
    p.set_int(0, static_cast<int32_t>(Op::CHECKPOINT));
    return lm.append(p.contents());
}

} // namespace tx
