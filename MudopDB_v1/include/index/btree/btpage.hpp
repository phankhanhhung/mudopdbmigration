#ifndef BTPAGE_HPP
#define BTPAGE_HPP

#include "file/blockid.hpp"
#include "query/constant.hpp"
#include "record/layout.hpp"
#include "record/rid.hpp"
#include <memory>
#include <optional>

namespace tx { class Transaction; }

namespace index {

/**
 * Low-level B-Tree page management.
 *
 * Page layout: [flag (4 bytes)][num_recs (4 bytes)][slot 0][slot 1]...
 *
 * Corresponds to BTPage in Rust (NMDB2/src/index/btree/btpage.rs)
 */
class BTPage {
public:
    BTPage(std::shared_ptr<tx::Transaction> tx,
           const file::BlockId& currentblk,
           const record::Layout& layout);

    int32_t find_slot_before(const Constant& searchkey);
    void close();
    bool is_full() const;
    file::BlockId split(size_t splitpos, int32_t flag);

    Constant get_data_val(size_t slot) const;
    int32_t get_flag() const;
    void set_flag(int32_t val);
    file::BlockId append_new(int32_t flag);
    void format(const file::BlockId& blk, int32_t flag);

    int32_t get_child_num(size_t slot) const;
    void insert_dir(size_t slot, const Constant& val, int32_t blknum);
    record::RID get_data_rid(size_t slot) const;
    void insert_leaf(size_t slot, const Constant& val, const record::RID& rid);
    void delete_entry(size_t slot);
    size_t get_num_recs() const;

private:
    int32_t get_int(size_t slot, const std::string& fldname) const;
    std::string get_string(size_t slot, const std::string& fldname) const;
    Constant get_val(size_t slot, const std::string& fldname) const;
    void set_int(size_t slot, const std::string& fldname, int32_t val);
    void set_string(size_t slot, const std::string& fldname, const std::string& val);
    void set_val(size_t slot, const std::string& fldname, const Constant& val);
    void set_num_recs(size_t n);
    void insert(size_t slot);
    void copy_record(size_t from, size_t to);
    void transfer_recs(size_t slot, BTPage& dest);
    size_t fldpos(size_t slot, const std::string& fldname) const;
    size_t slotpos(size_t slot) const;
    void make_default_record(const file::BlockId& blk, size_t pos);

    std::shared_ptr<tx::Transaction> tx_;
    std::optional<file::BlockId> currentblk_;
    record::Layout layout_;
};

} // namespace index

#endif // BTPAGE_HPP
