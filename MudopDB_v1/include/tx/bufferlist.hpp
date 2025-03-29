#ifndef BUFFERLIST_HPP
#define BUFFERLIST_HPP

#include "file/blockid.hpp"
#include "buffer/buffermgr.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>

namespace tx {

class BufferList {
public:
    explicit BufferList(std::shared_ptr<buffer::BufferMgr> bm);

    std::optional<size_t> get_index(const file::BlockId& blk) const;
    void pin(const file::BlockId& blk);
    void unpin(const file::BlockId& blk);
    void unpin_all();

private:
    std::shared_ptr<buffer::BufferMgr> bm_;
    std::unordered_map<file::BlockId, size_t> buffers_;
    std::vector<file::BlockId> pins_;
};

} // namespace tx

#endif // BUFFERLIST_HPP
