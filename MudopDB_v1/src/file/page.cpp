#include "file/page.hpp"
#include <cstring>
#include <algorithm>
#include <limits>

namespace file {

Page::Page(size_t blocksize) : bb_(blocksize, 0) {}

Page::Page(std::vector<uint8_t> data) : bb_(std::move(data)) {}

void Page::check_bounds(size_t offset, size_t size) const {
    if (offset + size > bb_.size()) {
        throw std::out_of_range("Page access out of bounds");
    }
}

int32_t Page::get_int(size_t offset) const {
    check_bounds(offset, 4);

    // Read 4 bytes in big-endian order
    int32_t result = 0;
    result |= static_cast<int32_t>(bb_[offset + 0]) << 24;
    result |= static_cast<int32_t>(bb_[offset + 1]) << 16;
    result |= static_cast<int32_t>(bb_[offset + 2]) << 8;
    result |= static_cast<int32_t>(bb_[offset + 3]) << 0;

    return result;
}

void Page::set_int(size_t offset, int32_t val) {
    check_bounds(offset, 4);

    // Write 4 bytes in big-endian order
    bb_[offset + 0] = static_cast<uint8_t>((val >> 24) & 0xFF);
    bb_[offset + 1] = static_cast<uint8_t>((val >> 16) & 0xFF);
    bb_[offset + 2] = static_cast<uint8_t>((val >> 8) & 0xFF);
    bb_[offset + 3] = static_cast<uint8_t>((val >> 0) & 0xFF);
}

const uint8_t* Page::get_bytes(size_t offset) const {
    check_bounds(offset, 4);
    int32_t length = get_int(offset);

    if (length < 0) {
        throw std::runtime_error("Invalid byte array length");
    }

    check_bounds(offset + 4, static_cast<size_t>(length));
    return &bb_[offset + 4];
}

size_t Page::get_bytes_length(size_t offset) const {
    check_bounds(offset, 4);
    int32_t length = get_int(offset);

    if (length < 0) {
        throw std::runtime_error("Invalid byte array length");
    }

    return static_cast<size_t>(length);
}

void Page::set_bytes(size_t offset, const uint8_t* data, size_t length) {
    if (length > static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
        throw std::invalid_argument("Byte array too large");
    }

    check_bounds(offset, 4 + length);

    // Write length prefix
    set_int(offset, static_cast<int32_t>(length));

    // Write data
    std::memcpy(&bb_[offset + 4], data, length);
}

std::string Page::get_string(size_t offset) const {
    const uint8_t* bytes = get_bytes(offset);
    size_t length = get_bytes_length(offset);

    return std::string(reinterpret_cast<const char*>(bytes), length);
}

void Page::set_string(size_t offset, const std::string& val) {
    set_bytes(offset, reinterpret_cast<const uint8_t*>(val.data()), val.length());
}

size_t Page::max_length(size_t strlen) {
    // 4 bytes for length prefix + string bytes
    // Assuming 1 byte per character (UTF-8, but for simplicity treating as 1 byte per char)
    return 4 + strlen;
}

size_t Page::size() const {
    return bb_.size();
}

std::vector<uint8_t>& Page::contents() {
    return bb_;
}

const std::vector<uint8_t>& Page::contents() const {
    return bb_;
}

} // namespace file
