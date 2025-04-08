#ifndef RID_HPP
#define RID_HPP

#include <string>
#include <cstdint>

namespace record {

class RID {
public:
    RID(int32_t blknum, size_t slot);

    int32_t block_number() const;

    size_t slot() const;

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
