#ifndef PAGE_HPP
#define PAGE_HPP

#include <vector>
#include <cstdint>
#include <string>
#include <stdexcept>

namespace file {

/**
 * Page represents an in-memory block of data.
 * It provides methods to read and write integers, strings, and byte arrays.
 * All integers are stored in big-endian (network byte order) format.
 *
 * Corresponds to Page in Rust (NMDB2/src/file/page.rs)
 */
class Page {
public:
    /**
     * Creates a new page of the specified size.
     * @param blocksize the size of the page in bytes
     */
    explicit Page(size_t blocksize);

    /**
     * Creates a page from existing data.
     * @param data the byte vector to use
     */
    explicit Page(std::vector<uint8_t> data);

    /**
     * Reads a 32-bit integer from the specified offset.
     * @param offset the byte offset within the page
     * @return the integer value in host byte order
     */
    int32_t get_int(size_t offset) const;

    /**
     * Writes a 32-bit integer to the specified offset.
     * @param offset the byte offset within the page
     * @param val the integer value to write
     */
    void set_int(size_t offset, int32_t val);

    /**
     * Reads a byte array from the specified offset.
     * The format is: [4-byte length][data bytes]
     * @param offset the byte offset within the page
     * @return pointer to the data bytes (not including length prefix)
     */
    const uint8_t* get_bytes(size_t offset) const;

    /**
     * Gets the length of the byte array at the specified offset.
     * @param offset the byte offset within the page
     * @return the length of the byte array
     */
    size_t get_bytes_length(size_t offset) const;

    /**
     * Writes a byte array to the specified offset.
     * The format is: [4-byte length][data bytes]
     * @param offset the byte offset within the page
     * @param data pointer to the data to write
     * @param length the number of bytes to write
     */
    void set_bytes(size_t offset, const uint8_t* data, size_t length);

    /**
     * Reads a string from the specified offset.
     * Strings are stored as byte arrays with UTF-8 encoding.
     * @param offset the byte offset within the page
     * @return the string value
     */
    std::string get_string(size_t offset) const;

    /**
     * Writes a string to the specified offset.
     * Strings are stored as byte arrays with UTF-8 encoding.
     * @param offset the byte offset within the page
     * @param val the string to write
     */
    void set_string(size_t offset, const std::string& val);

    /**
     * Calculates the maximum space needed to store a string of the given length.
     * @param strlen the maximum string length in characters
     * @return the number of bytes needed (4 + strlen)
     */
    static size_t max_length(size_t strlen);

    /**
     * Returns the size of the page in bytes.
     */
    size_t size() const;

    /**
     * Direct access to the underlying byte buffer (for I/O operations).
     * Use with caution.
     */
    std::vector<uint8_t>& contents();
    const std::vector<uint8_t>& contents() const;

private:
    std::vector<uint8_t> bb_;  // byte buffer

    // Helper to check bounds
    void check_bounds(size_t offset, size_t size) const;
};

} // namespace file

#endif // PAGE_HPP
