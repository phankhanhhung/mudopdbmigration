#ifndef BUFFERMGR_HPP
#define BUFFERMGR_HPP

#include "buffer/buffer.hpp"
#include "file/blockid.hpp"
#include "file/filemgr.hpp"
#include "log/logmgr.hpp"
#include <memory>
#include <vector>
#include <optional>
#include <stdexcept>

namespace buffer {

class BufferMgr {
public:
    BufferMgr(std::shared_ptr<file::FileMgr> fm,
              std::shared_ptr<log::LogMgr> lm,
              size_t numbuffs);

    size_t available() const;
    void flush_all(size_t txnum);
    size_t pin(const file::BlockId& blk);
    void unpin(size_t idx);
    Buffer& buffer(size_t idx);
    std::shared_ptr<file::FileMgr> file_mgr() const;

private:
    std::optional<size_t> try_to_pin(const file::BlockId& blk);
    std::optional<size_t> find_existing_buffer(const file::BlockId& blk);
    std::optional<size_t> choose_unpinned_buffer();

    std::shared_ptr<file::FileMgr> fm_;
    std::vector<Buffer> bufferpool_;
    size_t num_available_;
};

} // namespace buffer

#endif // BUFFERMGR_HPP
