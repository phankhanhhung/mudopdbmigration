#ifndef INDEXINFO_HPP
#define INDEXINFO_HPP

#include "metadata/statinfo.hpp"
#include "index/index.hpp"
#include "record/layout.hpp"
#include "record/schema.hpp"
#include <memory>
#include <string>

namespace tx { class Transaction; }

namespace metadata {

/**
 * IndexInfo holds metadata about a single index.
 *
 * Corresponds to IndexInfo in Rust (NMDB2/src/metadata/indexinfo.rs)
 */
class IndexInfo {
public:
    IndexInfo(const std::string& idxname,
              const std::string& fldname,
              std::shared_ptr<record::Schema> tbl_schema,
              std::shared_ptr<tx::Transaction> tx,
              StatInfo si);

    std::unique_ptr<::Index> open() const;
    size_t blocks_accessed() const;
    size_t records_output() const;
    size_t distinct_values(const std::string& fname) const;

private:
    static record::Layout create_idx_layout(const std::string& fldname,
                                             const record::Schema& tbl_schema);

    std::string idxname_;
    std::string fldname_;
    std::shared_ptr<tx::Transaction> tx_;
    record::Layout idx_layout_;
    StatInfo si_;
};

} // namespace metadata

#endif // INDEXINFO_HPP
