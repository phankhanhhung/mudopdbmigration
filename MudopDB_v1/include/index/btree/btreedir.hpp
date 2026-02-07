#ifndef BTREEDIR_HPP
#define BTREEDIR_HPP

#include "index/btree/btpage.hpp"
#include "index/btree/direntry.hpp"
#include "query/constant.hpp"
#include "record/layout.hpp"
#include "file/blockid.hpp"
#include <memory>
#include <optional>

namespace tx { class Transaction; }

namespace index {

/**
 * B-Tree directory (internal) node.
 *
 * Corresponds to BTreeDir in Rust (NMDB2/src/index/btree/btreedir.rs)
 */
class BTreeDir {
public:
    BTreeDir(std::shared_ptr<tx::Transaction> tx,
             const file::BlockId& blk,
             const record::Layout& layout);

    void close();
    int32_t search(const Constant& searchkey);
    void make_new_root(const DirEntry& e);
    std::optional<DirEntry> insert(const DirEntry& e);

private:
    std::optional<DirEntry> insert_entry(const DirEntry& e);
    file::BlockId find_child_block(const Constant& searchkey);

    std::shared_ptr<tx::Transaction> tx_;
    record::Layout layout_;
    BTPage contents_;
    std::string filename_;
};

} // namespace index

#endif // BTREEDIR_HPP
