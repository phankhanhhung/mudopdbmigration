#ifndef BTREEINDEX_HPP
#define BTREEINDEX_HPP

#include "index/index.hpp"
#include "index/btree/btreeleaf.hpp"
#include "index/btree/btreedir.hpp"
#include "record/layout.hpp"
#include "file/blockid.hpp"
#include <memory>
#include <optional>
#include <string>

namespace tx { class Transaction; }

namespace index {

/**
 * B-Tree index implementation.
 *
 * Corresponds to BTreeIndex in Rust (NMDB2/src/index/btree/btreeindex.rs)
 */
class BTreeIndex : public Index {
public:
    BTreeIndex(std::shared_ptr<tx::Transaction> tx,
               const std::string& idxname,
               const record::Layout& leaf_layout);

    static size_t search_cost(size_t numblocks, size_t rpb);

    void before_first(const Constant& searchkey) override;
    bool next() override;
    record::RID get_data_rid() override;
    void insert(const Constant& val, const record::RID& rid) override;
    void delete_entry(const Constant& val, const record::RID& rid) override;
    void close() override;

private:
    std::shared_ptr<tx::Transaction> tx_;
    record::Layout dir_layout_;
    record::Layout leaf_layout_;
    std::string leaftbl_;
    std::optional<BTreeLeaf> leaf_;
    file::BlockId rootblk_;
};

} // namespace index

#endif // BTREEINDEX_HPP
