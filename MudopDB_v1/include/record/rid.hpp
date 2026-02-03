#ifndef RID_HPP
#define RID_HPP

#include <string>
#include <cstdint>

namespace record {

/**
 * RID (Record ID) uniquely identifies a record.
 *
 * Format: [block_number, slot]
 *
 * Corresponds to Rid in Rust (NMDB2/src/record/rid.rs)
 */
class RID {
public:
    /**
     * Creates a record ID.
     *
     * @param blknum the block number
     * @param slot the slot number within the block
     */
    RID(int32_t blknum, size_t slot);

    /**
     * Returns the block number.
     *
     * @return block number
     */
    int32_t block_number() const;

    /**
     * Returns the slot number.
     *
     * @return slot number
     */
    size_t slot() const;

    /**
     * Returns string representation.
     *
     * @return format: "[blknum, slot]"
     */
    std::string to_string() const;

    // Equality operators
    bool operator==(const RID& other) const;
    bool operator!=(const RID& other) const;

private:
    int32_t blknum_;
    size_t slot_;
};

} // namespace record

#endif // RID_HPP
