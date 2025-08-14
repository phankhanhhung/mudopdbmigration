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

/**
 * Buffer wraps a Page with pinning and modification tracking.
 *
 * A buffer can be in one of three states:
 * 1. Unpinned and unassigned (blk = nullopt)
 * 2. Unpinned and assigned to a block
 * 3. Pinned (assigned to a block, pins > 0)
 *
 * The buffer maintains modification state for write-ahead logging:
 * - txnum: transaction that modified this buffer
 * - lsn: log sequence number of the modification
 *
 * When flushed, the buffer follows WAL protocol:
 * 1. Flush log first (if lsn is set)
 * 2. Then flush data page to disk
 *
 * Corresponds to Buffer in Rust (NMDB2/src/buffer/buffer.rs)
 *
 * Thread Safety: This class is NOT thread-safe. External synchronization
 * is required if used in multi-threaded context.
 */
class Buffer {
public:
    /**
     * Creates a new buffer.
     *
     * @param fm the file manager
     * @param lm the log manager
     */
    Buffer(std::shared_ptr<file::FileMgr> fm,
           std::shared_ptr<log::LogMgr> lm);

    /**
     * Returns a reference to the buffer's page contents.
     * Allows reading and writing to the page.
     *
     * @return reference to the page
     */
    file::Page& contents();

    /**
     * Returns the block this buffer is assigned to.
     *
     * @return the block, or std::nullopt if unassigned
     */
    const std::optional<file::BlockId>& block() const;

    /**
     * Marks the buffer as modified by the specified transaction.
     * If lsn is provided, it updates the buffer's LSN.
     *
     * @param txnum the modifying transaction number
     * @param lsn the log sequence number (optional)
     */
    void set_modified(size_t txnum, std::optional<size_t> lsn);

    /**
     * Returns whether the buffer is currently pinned.
     *
     * @return true if pins > 0
     */
    bool is_pinned() const;

    /**
     * Returns the transaction number that modified this buffer.
     *
     * @return the transaction number, or std::nullopt if not modified
     */
    std::optional<size_t> modifying_tx() const;

    /**
     * Assigns this buffer to a block.
     * Flushes the previous block if dirty, then reads the new block.
     * Resets pins to 0.
     *
     * NOTE: Package-private - should only be called by BufferMgr
     *
     * @param blk the block to assign
     */
    void assign_to_block(const file::BlockId& blk);

    /**
     * Flushes the buffer to disk if it has been modified.
     * Follows WAL: flushes log first if lsn is set.
     * Resets txnum to nullopt after flushing.
     *
     * NOTE: Package-private - should only be called by BufferMgr
     */
    void flush();

    /**
     * Increments the pin count.
     *
     * NOTE: Package-private - should only be called by BufferMgr
     */
    void pin();

    /**
     * Decrements the pin count.
     *
     * NOTE: Package-private - should only be called by BufferMgr
     */
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
