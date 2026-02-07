#include "index/btree/btreeindex.hpp"
#include "tx/transaction.hpp"
#include "record/schema.hpp"
#include <stdexcept>
#include <limits>

namespace index {

BTreeIndex::BTreeIndex(std::shared_ptr<tx::Transaction> tx,
                       const std::string& idxname,
                       const record::Layout& leaf_layout)
    : tx_(tx), dir_layout_(std::make_shared<record::Schema>()),
      leaf_layout_(leaf_layout),
      leaftbl_(idxname + "leaf"),
      rootblk_(idxname + "dir", 0) {

    if (tx_->size(leaftbl_) == 0) {
        auto blk = tx_->append(leaftbl_);
        BTPage node(tx_, blk, leaf_layout_);
        node.format(blk, -1);
    }

    auto dirsch = std::make_shared<record::Schema>();
    dirsch->add("block", *leaf_layout_.schema());
    dirsch->add("dataval", *leaf_layout_.schema());
    dir_layout_ = record::Layout(dirsch);

    std::string dirtbl = idxname + "dir";
    rootblk_ = file::BlockId(dirtbl, 0);
    if (tx_->size(dirtbl) == 0) {
        tx_->append(dirtbl);
        BTPage node(tx_, rootblk_, dir_layout_);
        node.format(rootblk_, 0);

        auto fldtype = dirsch->type("dataval");
        Constant minval = (fldtype == record::Type::INTEGER)
            ? Constant::with_int(std::numeric_limits<int32_t>::min())
            : Constant::with_string("");
        node.insert_dir(0, minval, 0);
        node.close();
    }
}

size_t BTreeIndex::search_cost(size_t numblocks, size_t rpb) {
    if (numblocks == 0 || rpb <= 1) return 1;
    // 1 + log_rpb(numblocks)
    size_t cost = 1;
    size_t n = numblocks;
    while (n > 1) {
        n /= rpb;
        cost++;
    }
    return cost;
}

void BTreeIndex::before_first(const Constant& searchkey) {
    close();
    BTreeDir root(tx_, rootblk_, dir_layout_);
    int32_t blknum = root.search(searchkey);
    root.close();
    file::BlockId leafblk(leaftbl_, blknum);
    leaf_.emplace(tx_, leafblk, leaf_layout_, searchkey);
}

bool BTreeIndex::next() {
    if (leaf_.has_value()) {
        return leaf_->next();
    }
    throw std::runtime_error("BTreeIndex: no leaf open");
}

record::RID BTreeIndex::get_data_rid() {
    if (leaf_.has_value()) {
        return leaf_->get_data_rid();
    }
    throw std::runtime_error("BTreeIndex: no leaf open");
}

void BTreeIndex::insert(const Constant& dataval, const record::RID& datarid) {
    before_first(dataval);
    if (leaf_.has_value()) {
        auto e = leaf_->insert(datarid);
        leaf_->close();
        if (e.has_value()) {
            BTreeDir root(tx_, rootblk_, dir_layout_);
            auto e2 = root.insert(e.value());
            if (e2.has_value()) {
                root.make_new_root(e2.value());
            }
            root.close();
        }
        return;
    }
    throw std::runtime_error("BTreeIndex: no leaf open");
}

void BTreeIndex::delete_entry(const Constant& dataval, const record::RID& datarid) {
    before_first(dataval);
    if (leaf_.has_value()) {
        leaf_->delete_entry(datarid);
        leaf_->close();
        return;
    }
    throw std::runtime_error("BTreeIndex: no leaf open");
}

void BTreeIndex::close() {
    if (leaf_.has_value()) {
        leaf_->close();
        leaf_.reset();
    }
}

} // namespace index
