#ifndef BUFFERMGR_HPP
#define BUFFERMGR_HPP

#include "buffer/buffer.hpp"
#include "file/blockid.hpp"
#include "file/filemgr.hpp"
#include "log/logmgr.hpp"
#include <memory>
#include <vector>
#include <optional>
#include <chrono>
#include <thread>
#include <stdexcept>

namespace buffer {

/**
 * Exception thrown when buffer pool is full and timeout expires.
 */
class BufferAbortException : public std::runtime_error {
public:
    BufferAbortException()
        : std::runtime_error("Buffer abort: pool exhausted after timeout") {}
};

/**
 * BufferMgr manages a fixed-size pool of buffers.
 *
 * Pin/Unpin Protocol:
 * - pin(blk) returns a buffer index, increments pin count
 * - unpin(idx) decrements pin count, makes buffer available
 * - Buffers with pins > 0 cannot be evicted
 *
 * Eviction Policy:
 * - Simple: first unpinned buffer found (naive strategy)
 * - Can be upgraded to clock algorithm or LRU later
 *
 * When pool is full:
 * - pin() waits up to MAX_TIME milliseconds
 * - Throws BufferAbortException if timeout expires
 *
 * Corresponds to BufferMgr in Rust (NMDB2/src/buffer/buffermgr.rs)
 *
 * Thread Safety: This class is NOT thread-safe. External synchronization
 * is required if used in multi-threaded context.
 */
class BufferMgr {
public:
    /**
     * Creates a buffer manager with the specified pool size.
     *
     * @param fm the file manager
     * @param lm the log manager
     * @param numbuffs the number of buffers in the pool
     */
    BufferMgr(std::shared_ptr<file::FileMgr> fm,
              std::shared_ptr<log::LogMgr> lm,
              size_t numbuffs);

    /**
     * Returns the number of available (unpinned) buffers.
     *
     * @return available buffer count
     */
    size_t available() const;

    /**
     * Flushes all buffers modified by the specified transaction.
     *
     * @param txnum the transaction number
     */
    void flush_all(size_t txnum);

    /**
     * Pins a buffer to the specified block.
     *
     * If the block is already in the pool, returns its index.
     * Otherwise, allocates an unpinned buffer and assigns it to the block.
     * If no unpinned buffers are available, waits up to max_time_ ms.
     *
     * @param blk the block to pin
     * @return the buffer index
     * @throws BufferAbortException if no buffer available after timeout
     */
    size_t pin(const file::BlockId& blk);

    /**
     * Unpins the buffer at the specified index.
     * Decrements the pin count and increases available count if needed.
     *
     * @param idx the buffer index
     */
    void unpin(size_t idx);

    /**
     * Returns a reference to the buffer at the specified index.
     *
     * @param idx the buffer index
     * @return reference to the buffer
     */
    Buffer& buffer(size_t idx);

    /**
     * Sets the maximum wait time in milliseconds.
     * (For testing purposes)
     *
     * @param max_time_ms the timeout in milliseconds
     */
    void set_max_time(uint64_t max_time_ms);

    /**
     * Returns the file manager.
     *
     * @return shared pointer to file manager
     */
    std::shared_ptr<file::FileMgr> file_mgr() const;

private:
    /**
     * Attempts to pin a buffer to the block without waiting.
     *
     * @param blk the block to pin
     * @return the buffer index, or std::nullopt if pool is full
     */
    std::optional<size_t> try_to_pin(const file::BlockId& blk);

    /**
     * Finds a buffer already assigned to the block.
     *
     * @param blk the block to find
     * @return the buffer index, or std::nullopt if not found
     */
    std::optional<size_t> find_existing_buffer(const file::BlockId& blk);

    /**
     * Chooses an unpinned buffer for eviction.
     * Simple strategy: first unpinned buffer found.
     *
     * @return the buffer index, or std::nullopt if all pinned
     */
    std::optional<size_t> choose_unpinned_buffer();

    /**
     * Checks if waiting time has exceeded the maximum.
     *
     * @param start_time the time when waiting started
     * @return true if timeout expired
     */
    bool waiting_too_long(
        const std::chrono::steady_clock::time_point& start_time) const;

private:
    static constexpr uint64_t MAX_TIME = 10000;  // 10 seconds in ms

    std::shared_ptr<file::FileMgr> fm_;
    std::vector<Buffer> bufferpool_;
    size_t num_available_;
    uint64_t max_time_;
};

} // namespace buffer

#endif // BUFFERMGR_HPP
