#include "file/filemgr.hpp"
#include <filesystem>
#include <stdexcept>
#include <cstring>

namespace fs = std::filesystem;

namespace file {

FileMgr::FileMgr(const std::string& db_directory, size_t blocksize)
    : db_directory_(db_directory), blocksize_(blocksize), is_new_(false) {

    // Check if directory exists
    is_new_ = !fs::exists(db_directory_);

    // Create directory if it doesn't exist
    if (is_new_) {
        fs::create_directories(db_directory_);
    }

    // Remove temporary files (files starting with "temp")
    if (fs::exists(db_directory_)) {
        for (const auto& entry : fs::directory_iterator(db_directory_)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find("temp") == 0) {
                    fs::remove(entry.path());
                }
            }
        }
    }
}

std::string FileMgr::get_file_path(const std::string& filename) const {
    return db_directory_ + "/" + filename;
}

std::fstream FileMgr::get_file(const std::string& filename, std::ios::openmode mode) {
    std::string filepath = get_file_path(filename);
    std::fstream file(filepath, mode);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    return file;
}

void FileMgr::update_file_size(const std::string& filename) {
    std::string filepath = get_file_path(filename);

    if (fs::exists(filepath)) {
        size_t file_size = fs::file_size(filepath);
        open_files_[filename] = file_size / blocksize_;
    } else {
        open_files_[filename] = 0;
    }
}

void FileMgr::read(const BlockId& blk, Page& page) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string filepath = get_file_path(blk.file_name());

    // Check if file and block exist
    if (!fs::exists(filepath)) {
        // File doesn't exist, page remains zeroed
        return;
    }

    std::fstream file = get_file(blk.file_name(), std::ios::in | std::ios::out | std::ios::binary);

    // Calculate position
    size_t pos = static_cast<size_t>(blk.number()) * blocksize_;

    // Get file size
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();

    // If block is beyond file size, page remains zeroed
    if (pos + page.contents().size() > file_size) {
        return;
    }

    // Seek and read
    file.seekg(pos, std::ios::beg);
    file.read(reinterpret_cast<char*>(page.contents().data()), page.contents().size());

    if (!file) {
        throw std::runtime_error("Failed to read block: " + blk.to_string());
    }
}

void FileMgr::write(const BlockId& blk, Page& page) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::fstream file = get_file(blk.file_name(), std::ios::in | std::ios::out | std::ios::binary);

    // Calculate position
    size_t pos = static_cast<size_t>(blk.number()) * blocksize_;

    // Seek and write
    file.seekp(pos, std::ios::beg);
    file.write(reinterpret_cast<const char*>(page.contents().data()), page.contents().size());

    if (!file) {
        throw std::runtime_error("Failed to write block: " + blk.to_string());
    }

    file.flush();

    // Update cached file size
    update_file_size(blk.file_name());
}

BlockId FileMgr::append(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Get current file size
    size_t new_blknum = length(filename);

    BlockId blk(filename, static_cast<int32_t>(new_blknum));

    // Create a zero-filled block
    std::vector<uint8_t> zeros(blocksize_, 0);

    std::fstream file = get_file(filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::app);

    // Seek to the position (should be at end, but explicit is safer)
    size_t pos = new_blknum * blocksize_;
    file.seekp(pos, std::ios::beg);

    // Write zeros
    file.write(reinterpret_cast<const char*>(zeros.data()), zeros.size());

    if (!file) {
        throw std::runtime_error("Failed to append block to: " + filename);
    }

    file.flush();

    // Update cached file size
    open_files_[filename] = new_blknum + 1;

    return blk;
}

size_t FileMgr::length(const std::string& filename) {
    // Check cache first
    auto it = open_files_.find(filename);
    if (it != open_files_.end()) {
        return it->second;
    }

    // Not in cache, read from filesystem
    std::string filepath = get_file_path(filename);

    if (!fs::exists(filepath)) {
        open_files_[filename] = 0;
        return 0;
    }

    size_t file_size = fs::file_size(filepath);
    size_t num_blocks = file_size / blocksize_;

    open_files_[filename] = num_blocks;
    return num_blocks;
}

bool FileMgr::is_new() const {
    return is_new_;
}

size_t FileMgr::block_size() const {
    return blocksize_;
}

} // namespace file
