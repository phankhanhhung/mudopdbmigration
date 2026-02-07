#ifndef HASHINDEX_HPP
#define HASHINDEX_HPP

#include "index/index.hpp"
#include "record/layout.hpp"
#include "record/tablescan.hpp"
#include "query/constant.hpp"
#include "record/rid.hpp"
#include <memory>
#include <optional>
#include <string>

namespace tx { class Transaction; }

namespace index {

/**
 * Hash-based index using 100 buckets.
 *
 * Corresponds to HashIndex in Rust (NMDB2/src/index/hash/hashindex.rs)
 */
class HashIndex : public Index {
public:
    static constexpr size_t NUM_BUCKETS = 100;

    HashIndex(std::shared_ptr<tx::Transaction> tx,
              const std::string& idxname,
              const record::Layout& layout);

    static size_t search_cost(size_t numblocks, size_t rpb);

    void before_first(const Constant& searchkey) override;
    bool next() override;
    record::RID get_data_rid() override;
    void insert(const Constant& val, const record::RID& rid) override;
    void delete_entry(const Constant& val, const record::RID& rid) override;
    void close() override;

private:
    std::shared_ptr<tx::Transaction> tx_;
    std::string idxname_;
    record::Layout layout_;
    std::optional<Constant> searchkey_;
    std::optional<record::TableScan> ts_;
};

} // namespace index

#endif // HASHINDEX_HPP
