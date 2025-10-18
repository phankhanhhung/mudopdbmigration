/**
 * StatInfo implementation.
 * Holds statistical information about a table: block count and record count.
 */

#include "metadata/statinfo.hpp"

namespace metadata {

StatInfo::StatInfo(size_t num_blocks, size_t num_recs)
    : num_blocks_(num_blocks), num_recs_(num_recs) {}

size_t StatInfo::blocks_accessed() const {
    return num_blocks_;
}

size_t StatInfo::records_output() const {
    return num_recs_;
}

size_t StatInfo::distinct_values(const std::string& /*fldname*/) const {
    return 1 + (num_recs_ / 3);
}

} // namespace metadata
