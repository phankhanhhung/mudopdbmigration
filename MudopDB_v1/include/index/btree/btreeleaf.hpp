#ifndef BTREELEAF_HPP
#define BTREELEAF_HPP

#include "index/btree/btpage.hpp"
#include "index/btree/direntry.hpp"
#include "query/constant.hpp"
#include "record/layout.hpp"
#include "record/rid.hpp"
#include "file/blockid.hpp"
#include <memory>
#include <optional>

namespace tx { class Transaction; }

namespace index {

/**
 * B-Tree leaf node.
 *
 * Corresponds to BTreeLeaf in Rust (NMDB2/src/index/btree/btreeleaf.rs)
 */
class BTreeLeaf {
public:
    BTreeLeaf(std::shared_ptr<tx::Transaction> tx,
              const file::BlockId& blk,
              const record::Layout& layout,
              const Constant& searchkey);

    void close();
    bool next();
    record::RID get_data_rid() const;
    void delete_entry(const record::RID& dataid);
    std::optional<DirEntry> insert(const record::RID& datarid);

private:
    bool try_overflow();

    std::shared_ptr<tx::Transaction> tx_;
    record::Layout layout_;
    Constant searchkey_;
    BTPage contents_;
    int32_t currentslot_;
    std::string filename_;
};

} // namespace index

#endif // BTREELEAF_HPP
