#include "index/btree/btreeleaf.hpp"
#include "tx/transaction.hpp"

namespace index {

BTreeLeaf::BTreeLeaf(std::shared_ptr<tx::Transaction> tx,
                     const file::BlockId& blk,
                     const record::Layout& layout,
                     const Constant& searchkey)
    : tx_(tx), layout_(layout), searchkey_(searchkey),
      contents_(tx, blk, layout), filename_(blk.file_name()) {
    currentslot_ = contents_.find_slot_before(searchkey);
}

void BTreeLeaf::close() {
    contents_.close();
}

bool BTreeLeaf::next() {
    currentslot_++;
    if (currentslot_ >= static_cast<int32_t>(contents_.get_num_recs())) {
        return try_overflow();
    }
    if (contents_.get_data_val(currentslot_) == searchkey_) {
        return true;
    }
    return try_overflow();
}

record::RID BTreeLeaf::get_data_rid() const {
    return contents_.get_data_rid(currentslot_);
}

void BTreeLeaf::delete_entry(const record::RID& dataid) {
    while (next()) {
        if (get_data_rid() == dataid) {
            contents_.delete_entry(currentslot_);
            return;
        }
    }
}

std::optional<DirEntry> BTreeLeaf::insert(const record::RID& datarid) {
    if (contents_.get_flag() >= 0 && contents_.get_data_val(0) > searchkey_) {
        Constant firstval = contents_.get_data_val(0);
        auto newblk = contents_.split(0, contents_.get_flag());
        currentslot_ = 0;
        contents_.set_flag(-1);
        contents_.insert_leaf(currentslot_, searchkey_, datarid);
        return DirEntry(firstval, newblk.number());
    }

    currentslot_++;
    contents_.insert_leaf(currentslot_, searchkey_, datarid);
    if (!contents_.is_full()) {
        return std::nullopt;
    }

    Constant firstkey = contents_.get_data_val(0);
    Constant lastkey = contents_.get_data_val(contents_.get_num_recs() - 1);
    if (lastkey == firstkey) {
        auto newblk = contents_.split(1, contents_.get_flag());
        contents_.set_flag(newblk.number());
        return std::nullopt;
    }
    size_t splitpos = contents_.get_num_recs() / 2;
    Constant splitkey = contents_.get_data_val(splitpos);
    if (splitkey == firstkey) {
        while (contents_.get_data_val(splitpos) == splitkey) {
            splitpos++;
        }
        splitkey = contents_.get_data_val(splitpos);
    } else {
        while (contents_.get_data_val(splitpos - 1) == splitkey) {
            splitpos--;
        }
    }
    auto newblk = contents_.split(splitpos, -1);
    return DirEntry(splitkey, newblk.number());
}

bool BTreeLeaf::try_overflow() {
    Constant firstkey = contents_.get_data_val(0);
    int32_t flag = contents_.get_flag();
    if (searchkey_ != firstkey || flag < 0) {
        return false;
    }
    contents_.close();
    file::BlockId nextblk(filename_, flag);
    contents_ = BTPage(tx_, nextblk, layout_);
    currentslot_ = 0;
    return true;
}

} // namespace index
