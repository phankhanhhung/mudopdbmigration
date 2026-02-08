#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace protocol {

// Request message types
enum class MsgType : uint8_t {
    CONN_CLOSE       = 0x01,
    CONN_COMMIT      = 0x02,
    CONN_ROLLBACK    = 0x03,
    STMT_EXEC_QUERY  = 0x11,
    STMT_EXEC_UPDATE = 0x12,
    RS_NEXT          = 0x21,
    RS_GET_INT       = 0x22,
    RS_GET_STRING    = 0x23,
    RS_CLOSE         = 0x24,
    MD_COL_COUNT     = 0x31,
    MD_COL_NAME      = 0x32,
    MD_COL_TYPE      = 0x33,
    MD_COL_DISPLAY   = 0x34,
};

// Response status
enum class Status : uint8_t {
    OK    = 0x00,
    ERROR = 0xFF,
};

// Buffer for encoding/decoding messages
class Buffer {
public:
    Buffer() = default;
    explicit Buffer(std::vector<uint8_t> data) : data_(std::move(data)) {}

    // Encoding (write)
    void write_uint8(uint8_t v) { data_.push_back(v); }

    void write_uint32(uint32_t v) {
        data_.push_back((v >> 24) & 0xFF);
        data_.push_back((v >> 16) & 0xFF);
        data_.push_back((v >> 8) & 0xFF);
        data_.push_back(v & 0xFF);
    }

    void write_int32(int32_t v) { write_uint32(static_cast<uint32_t>(v)); }

    void write_uint64(uint64_t v) {
        data_.push_back((v >> 56) & 0xFF);
        data_.push_back((v >> 48) & 0xFF);
        data_.push_back((v >> 40) & 0xFF);
        data_.push_back((v >> 32) & 0xFF);
        data_.push_back((v >> 24) & 0xFF);
        data_.push_back((v >> 16) & 0xFF);
        data_.push_back((v >> 8) & 0xFF);
        data_.push_back(v & 0xFF);
    }

    void write_bool(bool v) { data_.push_back(v ? 1 : 0); }

    void write_string(const std::string& s) {
        write_uint32(static_cast<uint32_t>(s.size()));
        data_.insert(data_.end(), s.begin(), s.end());
    }

    // Decoding (read)
    uint8_t read_uint8() {
        check(1);
        return data_[pos_++];
    }

    uint32_t read_uint32() {
        check(4);
        uint32_t v = (static_cast<uint32_t>(data_[pos_]) << 24) |
                     (static_cast<uint32_t>(data_[pos_ + 1]) << 16) |
                     (static_cast<uint32_t>(data_[pos_ + 2]) << 8) |
                     static_cast<uint32_t>(data_[pos_ + 3]);
        pos_ += 4;
        return v;
    }

    int32_t read_int32() { return static_cast<int32_t>(read_uint32()); }

    uint64_t read_uint64() {
        check(8);
        uint64_t v = 0;
        for (int i = 0; i < 8; i++) {
            v = (v << 8) | data_[pos_++];
        }
        return v;
    }

    bool read_bool() { return read_uint8() != 0; }

    std::string read_string() {
        uint32_t len = read_uint32();
        check(len);
        std::string s(data_.begin() + pos_, data_.begin() + pos_ + len);
        pos_ += len;
        return s;
    }

    const std::vector<uint8_t>& data() const { return data_; }
    size_t size() const { return data_.size(); }
    void reset_read() { pos_ = 0; }

private:
    void check(size_t n) const {
        if (pos_ + n > data_.size()) {
            throw std::runtime_error("protocol::Buffer: read past end");
        }
    }

    std::vector<uint8_t> data_;
    size_t pos_ = 0;
};

} // namespace protocol

#endif // PROTOCOL_HPP
