#ifndef BLOCKID_HPP
#define BLOCKID_HPP

#include <string>
#include <functional>

namespace file {

/**
 * BlockId uniquely identifies a block in the file system.
 * A block is identified by its filename and block number.
 *
 * Corresponds to BlockId in Rust (NMDB2/src/file/blockid.rs)
 */
class BlockId {
public:
    /**
     * Creates a new block identifier.
     * @param filename the name of the file
     * @param blknum the block number within the file
     */
    BlockId(const std::string& filename, int32_t blknum);

    /**
     * Returns the name of the file where this block is located.
     */
    const std::string& file_name() const;

    /**
     * Returns the block number within the file.
     */
    int32_t number() const;

    /**
     * String representation for debugging.
     */
    std::string to_string() const;

    // Equality comparison
    bool operator==(const BlockId& other) const;
    bool operator!=(const BlockId& other) const;

    // For use in ordered containers (std::map, std::set)
    bool operator<(const BlockId& other) const;

private:
    std::string filename_;
    int32_t blknum_;
};

} // namespace file

// Hash function for BlockId to enable use in std::unordered_map and std::unordered_set
namespace std {
    template<>
    struct hash<file::BlockId> {
        size_t operator()(const file::BlockId& blk) const noexcept;
    };
}

#endif // BLOCKID_HPP
