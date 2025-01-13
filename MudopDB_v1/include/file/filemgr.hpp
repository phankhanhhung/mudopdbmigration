#ifndef FILEMGR_HPP
#define FILEMGR_HPP

#include "file/blockid.hpp"
#include "file/page.hpp"
#include <string>
#include <unordered_map>
#include <fstream>

namespace file {

class FileMgr {
public:
    FileMgr(const std::string& db_directory, size_t blocksize);
    void read(const BlockId& blk, Page& page);
    void write(const BlockId& blk, Page& page);
    BlockId append(const std::string& filename);
    size_t length(const std::string& filename);
    bool is_new() const;
    size_t block_size() const;

private:
    std::string db_directory_;
    size_t blocksize_;
    bool is_new_;
    std::unordered_map<std::string, size_t> open_files_;

    std::string get_file_path(const std::string& filename) const;
    std::fstream get_file(const std::string& filename, std::ios::openmode mode);
    void update_file_size(const std::string& filename);
};

} // namespace file

#endif // FILEMGR_HPP
