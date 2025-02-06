#ifndef BUFFER_HPP
#define BUFFER_HPP

#include "file/page.hpp"
#include "file/blockid.hpp"
#include "file/filemgr.hpp"
#include "log/logmgr.hpp"
#include <memory>
#include <optional>
#include <cstdint>

namespace buffer {

class Buffer {
public:
    Buffer(std::shared_ptr<file::FileMgr> fm,
           std::shared_ptr<log::LogMgr> lm);

    file::Page& contents();

    const std::optional<file::BlockId>& block() const;

    void set_modified(size_t txnum, std::optional<size_t> lsn);

    bool is_pinned() const;

    std::optional<size_t> modifying_tx() const;

    void assign_to_block(const file::BlockId& blk);

    void flush();

    void pin();

    void unpin();

private:
    std::shared_ptr<file::FileMgr> fm_;
    std::shared_ptr<log::LogMgr> lm_;
    file::Page contents_;
    std::optional<file::BlockId> blk_;
    int32_t pins_;
    std::optional<size_t> txnum_;
    std::optional<size_t> lsn_;
};

} // namespace buffer

#endif // BUFFER_HPP
