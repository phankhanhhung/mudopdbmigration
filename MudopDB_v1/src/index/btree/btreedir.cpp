#include "index/btree/btreedir.hpp"
#include "tx/transaction.hpp"

namespace index {

BTreeDir::BTreeDir(std::shared_ptr<tx::Transaction> tx,
                   const file::BlockId& blk,
                   const record::Layout& layout)
    : tx_(tx), layout_(layout), contents_(tx, blk, layout),
      filename_(blk.file_name()) {}

void BTreeDir::close() {
    contents_.close();
}

int32_t BTreeDir::search(const Constant& searchkey) {
    auto childblk = find_child_block(searchkey);
    while (contents_.get_flag() > 0) {
        contents_.close();
        contents_ = BTPage(tx_, childblk, layout_);
        childblk = find_child_block(searchkey);
    }
    return childblk.number();
}

void BTreeDir::make_new_root(const DirEntry& e) {
    Constant firstval = contents_.get_data_val(0);
    int32_t level = contents_.get_flag();
    auto newblk = contents_.split(0, level);
    DirEntry oldroot(firstval, newblk.number());
    insert_entry(oldroot);
    insert_entry(e);
    contents_.set_flag(level + 1);
}

std::optional<DirEntry> BTreeDir::insert(const DirEntry& e) {
    if (contents_.get_flag() == 0) {
        return insert_entry(e);
    }
    auto childblk = find_child_block(e.data_val());
    BTreeDir child(tx_, childblk, layout_);
    auto myentry = child.insert(e);
    child.close();
    if (myentry.has_value()) {
        return insert_entry(myentry.value());
    }
    return std::nullopt;
}

std::optional<DirEntry> BTreeDir::insert_entry(const DirEntry& e) {
    size_t newslot = static_cast<size_t>(1 + contents_.find_slot_before(e.data_val()));
    contents_.insert_dir(newslot, e.data_val(), e.block_number());
    if (!contents_.is_full()) {
        return std::nullopt;
    }
    int32_t level = contents_.get_flag();
    size_t splitpos = contents_.get_num_recs() / 2;
    Constant splitval = contents_.get_data_val(splitpos);
    auto newblk = contents_.split(splitpos, level);
    return DirEntry(splitval, newblk.number());
}

file::BlockId BTreeDir::find_child_block(const Constant& searchkey) {
    int32_t slot = contents_.find_slot_before(searchkey);
    if (contents_.get_data_val(static_cast<size_t>(slot + 1)) == searchkey) {
        slot++;
    }
    int32_t blknum = contents_.get_child_num(static_cast<size_t>(slot));
    return file::BlockId(filename_, blknum);
}

} // namespace index
