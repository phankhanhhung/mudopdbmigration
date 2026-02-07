#include "tx/recovery/recoverymgr.hpp"
#include "tx/recovery/startrecord.hpp"
#include "tx/recovery/commitrecord.hpp"
#include "tx/recovery/setintrecord.hpp"
#include "tx/recovery/setstringrecord.hpp"
#include <stdexcept>

namespace tx {

RecoveryMgr::RecoveryMgr(size_t txnum,
                         std::shared_ptr<log::LogMgr> lm,
                         std::shared_ptr<buffer::BufferMgr> bm)
    : lm_(lm), bm_(bm), txnum_(txnum) {
    StartRecord::write_to_log(*lm_, txnum_);
}

void RecoveryMgr::commit() {
    bm_->flush_all(txnum_);
    size_t lsn = CommitRecord::write_to_log(*lm_, txnum_);
    lm_->flush(lsn);
}

size_t RecoveryMgr::set_int(buffer::Buffer& buff, size_t offset, int32_t /*newval*/) {
    int32_t oldval = buff.contents().get_int(offset);
    auto blk = buff.block();
    if (blk.has_value()) {
        return SetIntRecord::write_to_log(*lm_, txnum_, blk.value(), offset, oldval);
    }
    throw std::runtime_error("RecoveryMgr::set_int: buffer has no block");
}

size_t RecoveryMgr::set_string(buffer::Buffer& buff, size_t offset,
                                const std::string& /*newval*/) {
    std::string oldval = buff.contents().get_string(offset);
    auto blk = buff.block();
    if (blk.has_value()) {
        return SetStringRecord::write_to_log(*lm_, txnum_, blk.value(), offset, oldval);
    }
    throw std::runtime_error("RecoveryMgr::set_string: buffer has no block");
}

} // namespace tx
