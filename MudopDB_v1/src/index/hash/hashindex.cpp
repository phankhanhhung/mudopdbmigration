#include "index/hash/hashindex.hpp"
#include "tx/transaction.hpp"
#include <functional>
#include <stdexcept>

namespace index {

HashIndex::HashIndex(std::shared_ptr<tx::Transaction> tx,
                     const std::string& idxname,
                     const record::Layout& layout)
    : tx_(tx), idxname_(idxname), layout_(layout) {}

size_t HashIndex::search_cost(size_t numblocks, size_t /*rpb*/) {
    return numblocks / NUM_BUCKETS;
}

void HashIndex::before_first(const Constant& searchkey) {
    close();
    searchkey_ = searchkey;
    size_t bucket = std::hash<Constant>{}(searchkey) % NUM_BUCKETS;
    std::string tblname = idxname_ + std::to_string(bucket);
    ts_.emplace(tx_, tblname, layout_);
}

bool HashIndex::next() {
    if (ts_.has_value() && searchkey_.has_value()) {
        while (ts_->next()) {
            if (ts_->get_val("dataval") == searchkey_.value()) {
                return true;
            }
        }
    }
    return false;
}

record::RID HashIndex::get_data_rid() {
    if (ts_.has_value()) {
        int32_t blknum = ts_->get_int("block");
        int32_t id = ts_->get_int("id");
        return record::RID(blknum, id);
    }
    throw std::runtime_error("HashIndex: no table scan open");
}

void HashIndex::insert(const Constant& val, const record::RID& rid) {
    before_first(val);
    if (ts_.has_value()) {
        ts_->insert();
        ts_->set_int("block", rid.block_number());
        ts_->set_int("id", rid.slot());
        ts_->set_val("dataval", val);
        return;
    }
    throw std::runtime_error("HashIndex: no table scan open");
}

void HashIndex::delete_entry(const Constant& val, const record::RID& rid) {
    before_first(val);
    while (next()) {
        if (get_data_rid() == rid) {
            if (ts_.has_value()) {
                ts_->delete_record();
            }
            return;
        }
    }
}

void HashIndex::close() {
    if (ts_.has_value()) {
        ts_->close();
        ts_.reset();
    }
}

} // namespace index
