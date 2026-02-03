#ifndef LOGMGR_HPP
#define LOGMGR_HPP

#include "file/blockid.hpp"
#include "file/page.hpp"
#include "file/filemgr.hpp"
#include "log/logiterator.hpp"
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <mutex>

namespace log {

/**
 * LogMgr manages the write-ahead log (WAL) for the database.
 *
 * The log uses a backward-growing format within each page:
 * - Offset 0: boundary (4 bytes) - position of first free byte
 * - Records grow from end toward beginning
 * - Each record: [4-byte length][data bytes]
 *
 * Log Sequence Numbers (LSN) are monotonically increasing and used
 * to track which log records have been flushed to disk.
 *
 * Corresponds to LogMgr in Rust (NMDB2/src/log/logmgr.rs)
 *
 * Thread Safety: This class is NOT thread-safe. Callers must
 * synchronize access (typically via mutex in Transaction layer).
 */
class LogMgr {
public:
    /**
     * Creates a log manager for the specified file.
     * If the log file doesn't exist, a new one is created.
     * If it exists, the last block is loaded.
     *
     * @param fm the file manager
     * @param logfile the name of the log file
     */
    LogMgr(std::shared_ptr<file::FileMgr> fm, const std::string& logfile);

    /**
     * Appends a log record to the log.
     * If the record doesn't fit in the current page, a new page is allocated.
     *
     * @param logrec the log record as a byte vector
     * @return the LSN (Log Sequence Number) of the appended record
     */
    size_t append(const std::vector<uint8_t>& logrec);

    /**
     * Flushes the log to disk if the specified LSN has not been saved yet.
     * This ensures write-ahead logging: log records are on disk before
     * corresponding data pages.
     *
     * @param lsn the log sequence number to flush
     */
    void flush(size_t lsn);

    /**
     * Creates an iterator to read log records backward from most recent.
     * The log is flushed before creating the iterator.
     *
     * @return a log iterator
     */
    std::unique_ptr<LogIterator> iterator();

private:
    std::shared_ptr<file::FileMgr> fm_;
    std::string logfile_;
    file::Page logpage_;
    file::BlockId currentblk_;
    size_t latest_lsn_;
    size_t last_saved_lsn_;

    /**
     * Allocates a new log block and formats it.
     * @return the new block identifier
     */
    file::BlockId append_new_block();

    /**
     * Writes the current log page to disk.
     */
    void flush_impl();

    /**
     * Helper function to append a new block and format it.
     */
    static file::BlockId append_new_block_helper(
        file::FileMgr* fm,
        file::Page& logpage,
        const std::string& logfile
    );
};

} // namespace log

#endif // LOGMGR_HPP
