#include "metadata/indexinfo.hpp"
#include "index/btree/btreeindex.hpp"
#include "tx/transaction.hpp"

namespace metadata {

record::Layout IndexInfo::create_idx_layout(const std::string& fldname,
                                             const record::Schema& tbl_schema) {
    auto sch = std::make_shared<record::Schema>();
    sch->add_int_field("block");
    sch->add_int_field("id");
    if (tbl_schema.type(fldname) == record::Type::INTEGER) {
        sch->add_int_field("dataval");
    } else {
        size_t fldlen = tbl_schema.length(fldname);
        sch->add_string_field("dataval", fldlen);
    }
    return record::Layout(sch);
}

IndexInfo::IndexInfo(const std::string& idxname,
                     const std::string& fldname,
                     std::shared_ptr<record::Schema> tbl_schema,
                     std::shared_ptr<tx::Transaction> tx,
                     StatInfo si)
    : idxname_(idxname), fldname_(fldname), tx_(tx),
      idx_layout_(create_idx_layout(fldname, *tbl_schema)), si_(si) {}

std::unique_ptr<::Index> IndexInfo::open() const {
    return std::make_unique<index::BTreeIndex>(tx_, idxname_, idx_layout_);
}

size_t IndexInfo::blocks_accessed() const {
    size_t rpb = tx_->block_size() / idx_layout_.slot_size();
    if (rpb == 0) rpb = 1;
    size_t numblocks = si_.records_output() / rpb;
    return index::BTreeIndex::search_cost(numblocks, rpb);
}

size_t IndexInfo::records_output() const {
    size_t dv = si_.distinct_values(fldname_);
    if (dv == 0) dv = 1;
    return si_.records_output() / dv;
}

size_t IndexInfo::distinct_values(const std::string& fname) const {
    if (fldname_ == fname) {
        return 1;
    }
    return si_.distinct_values(fldname_);
}

} // namespace metadata
