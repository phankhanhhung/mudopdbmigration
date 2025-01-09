#ifndef PAGE_HPP
#define PAGE_HPP

#include <vector>
#include <cstdint>
#include <string>
#include <stdexcept>

namespace file {

class Page {
public:
    explicit Page(size_t blocksize);

    explicit Page(std::vector<uint8_t> data);

    int32_t get_int(size_t offset) const;

    void set_int(size_t offset, int32_t val);

    const uint8_t* get_bytes(size_t offset) const;

    size_t get_bytes_length(size_t offset) const;

    void set_bytes(size_t offset, const uint8_t* data, size_t length);

    std::string get_string(size_t offset) const;

    void set_string(size_t offset, const std::string& val);

    static size_t max_length(size_t strlen);

    size_t size() const;

    std::vector<uint8_t>& contents();
    const std::vector<uint8_t>& contents() const;

private:
    std::vector<uint8_t> bb_;  // byte buffer

    // Helper to check bounds
    void check_bounds(size_t offset, size_t size) const;
};

} // namespace file

#endif // PAGE_HPP
