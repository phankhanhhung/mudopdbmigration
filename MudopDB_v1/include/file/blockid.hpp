#ifndef BLOCKID_HPP
#define BLOCKID_HPP

#include <string>
#include <functional>

namespace file {

class BlockId {
public:
    BlockId(const std::string& filename, int32_t blknum);

    const std::string& file_name() const;

    int32_t number() const;

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
