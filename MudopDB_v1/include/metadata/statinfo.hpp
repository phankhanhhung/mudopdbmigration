#ifndef STATINFO_HPP
#define STATINFO_HPP

#include <string>
#include <cstddef>

namespace metadata {

/**
 * StatInfo holds statistical information about a table.
 *
 * Corresponds to StatInfo in Rust (NMDB2/src/metadata/statinfo.rs)
 */
class StatInfo {
public:
    StatInfo(size_t num_blocks, size_t num_recs);

    size_t blocks_accessed() const;
    size_t records_output() const;
    size_t distinct_values(const std::string& fldname) const;

private:
    size_t num_blocks_;
    size_t num_recs_;
};

} // namespace metadata

#endif // STATINFO_HPP
