#ifndef FILEMGR_HPP
#define FILEMGR_HPP

#include "file/blockid.hpp"
#include "file/page.hpp"
#include <string>
#include <unordered_map>
#include <mutex>
#include <fstream>

namespace file {

/**
 * FileMgr manages the database files on disk.
 * It provides methods to read and write disk blocks.
 * All file operations are thread-safe.
 *
 * Corresponds to FileMgr in Rust (NMDB2/src/file/filemgr.rs)
 */
class FileMgr {
public:
    /**
     * Creates a file manager for the specified database directory.
     * If the directory doesn't exist, it will be created.
     * Temporary files (starting with "temp") are deleted on startup.
     *
     * @param db_directory the directory where database files are stored
     * @param blocksize the size of each block in bytes
     */
    FileMgr(const std::string& db_directory, size_t blocksize);

    /**
     * Reads a block from disk into the provided page.
     * If the block doesn't exist yet, the page is left with zeros.
     *
     * @param blk the block identifier
     * @param page the page to read into
     */
    void read(const BlockId& blk, Page& page);

    /**
     * Writes the contents of a page to disk.
     *
     * @param blk the block identifier
     * @param page the page to write
     */
    void write(const BlockId& blk, Page& page);

    /**
     * Appends a new block to the end of the specified file.
     *
     * @param filename the name of the file
     * @return the block identifier of the newly appended block
     */
    BlockId append(const std::string& filename);

    /**
     * Returns the number of blocks in the specified file.
     *
     * @param filename the name of the file
     * @return the number of blocks (file size / blocksize)
     */
    size_t length(const std::string& filename);

    /**
     * Returns true if this database was newly created.
     */
    bool is_new() const;

    /**
     * Returns the block size in bytes.
     */
    size_t block_size() const;

private:
    std::string db_directory_;
    size_t blocksize_;
    bool is_new_;
    std::unordered_map<std::string, size_t> open_files_;  // filename -> size in blocks
    mutable std::mutex mutex_;  // Protect file operations

    /**
     * Gets the full path to a database file.
     */
    std::string get_file_path(const std::string& filename) const;

    /**
     * Opens or creates a file and returns a stream.
     */
    std::fstream get_file(const std::string& filename, std::ios::openmode mode);

    /**
     * Updates the cached file size.
     */
    void update_file_size(const std::string& filename);
};

} // namespace file

#endif // FILEMGR_HPP
