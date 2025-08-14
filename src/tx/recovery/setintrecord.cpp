#include "tx/recovery/setintrecord.hpp"
#include "tx/transaction.hpp"

namespace tx {

SetIntRecord::SetIntRecord(const file::Page& p)
    : blk_("", 0) {
    constexpr size_t bytes = 4;
    size_t tpos = bytes;
    txnum_ = static_cast<size_t>(p.get_int(tpos));
    size_t fpos = tpos + bytes;
    std::string filename = p.get_string(fpos);
    size_t bpos = fpos + file::Page::max_length(filename.length());
    int32_t blknum = p.get_int(bpos);
    blk_ = file::BlockId(filename, blknum);
    size_t opos = bpos + bytes;
    offset_ = static_cast<size_t>(p.get_int(opos));
    size_t vpos = opos + bytes;
    val_ = p.get_int(vpos);
}

Op SetIntRecord::op() const {
    return Op::SETINT;
}

std::optional<size_t> SetIntRecord::tx_number() const {
    return txnum_;
}

void SetIntRecord::undo(Transaction& tx) {
    tx.pin(blk_);
    tx.set_int(blk_, offset_, val_, false);
    tx.unpin(blk_);
}

size_t SetIntRecord::write_to_log(log::LogMgr& lm, size_t txnum,
                                   const file::BlockId& blk, size_t offset,
                                   int32_t val) {
    constexpr size_t bytes = 4;
    size_t tpos = bytes;
    size_t fpos = tpos + bytes;
    size_t bpos = fpos + file::Page::max_length(blk.file_name().length());
    size_t opos = bpos + bytes;
    size_t vpos = opos + bytes;
    std::vector<uint8_t> rec(vpos + bytes, 0);
    file::Page p(std::move(rec));
    p.set_int(0, static_cast<int32_t>(Op::SETINT));
    p.set_int(tpos, static_cast<int32_t>(txnum));
    p.set_string(fpos, blk.file_name());
    p.set_int(bpos, blk.number());
    p.set_int(opos, static_cast<int32_t>(offset));
    p.set_int(vpos, val);
    return lm.append(p.contents());
}

} // namespace tx
