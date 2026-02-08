#include "api/metadata.hpp"
#include "api/connection.hpp"
#include "server/tcp_transport.hpp"
#include "server/protocol.hpp"
#include <algorithm>
#include <memory>
#include <string>

// ============================================================================
// EmbeddedMetadata
// ============================================================================

EmbeddedMetadata::EmbeddedMetadata(std::shared_ptr<record::Schema> schema)
    : sch_(std::move(schema)) {}

size_t EmbeddedMetadata::get_column_count() const {
    return sch_ ? sch_->fields().size() : 0;
}

std::string EmbeddedMetadata::get_column_name(size_t column) const {
    if (!sch_ || column == 0 || column > sch_->fields().size()) return {};
    return sch_->fields()[column - 1];
}

record::Type EmbeddedMetadata::get_column_type(size_t column) const {
    std::string fldname = get_column_name(column);
    if (fldname.empty() || !sch_) return record::Type::INTEGER;
    return sch_->type(fldname);
}

size_t EmbeddedMetadata::get_column_display_size(size_t column) const {
    std::string fldname = get_column_name(column);
    if (fldname.empty() || !sch_) return 0;
    record::Type fldtype = sch_->type(fldname);
    size_t fldlength;
    if (fldtype == record::Type::INTEGER) {
        fldlength = 6;
    } else {
        fldlength = sch_->length(fldname);
    }
    return std::max(fldname.size(), fldlength) + 1;
}

// ============================================================================
// NetworkMetadata
// ============================================================================

NetworkMetadata::NetworkMetadata(std::shared_ptr<NetworkConnection> conn, uint64_t id)
    : conn_(std::move(conn)), id_(id) {}

size_t NetworkMetadata::get_column_count() const {
    protocol::Buffer req;
    req.write_uint8(static_cast<uint8_t>(protocol::MsgType::MD_COL_COUNT));
    req.write_uint64(id_);
    auto resp = conn_->channel()->rpc(req);
    auto status = static_cast<protocol::Status>(resp.read_uint8());
    if (status == protocol::Status::ERROR) {
        throw std::runtime_error(resp.read_string());
    }
    return static_cast<size_t>(resp.read_uint64());
}

std::string NetworkMetadata::get_column_name(size_t column) const {
    protocol::Buffer req;
    req.write_uint8(static_cast<uint8_t>(protocol::MsgType::MD_COL_NAME));
    req.write_uint64(id_);
    req.write_uint64(static_cast<uint64_t>(column));
    auto resp = conn_->channel()->rpc(req);
    auto status = static_cast<protocol::Status>(resp.read_uint8());
    if (status == protocol::Status::ERROR) {
        throw std::runtime_error(resp.read_string());
    }
    return resp.read_string();
}

record::Type NetworkMetadata::get_column_type(size_t column) const {
    protocol::Buffer req;
    req.write_uint8(static_cast<uint8_t>(protocol::MsgType::MD_COL_TYPE));
    req.write_uint64(id_);
    req.write_uint64(static_cast<uint64_t>(column));
    auto resp = conn_->channel()->rpc(req);
    auto status = static_cast<protocol::Status>(resp.read_uint8());
    if (status == protocol::Status::ERROR) {
        throw std::runtime_error(resp.read_string());
    }
    int32_t type_val = resp.read_int32();
    return static_cast<record::Type>(type_val);
}

size_t NetworkMetadata::get_column_display_size(size_t column) const {
    protocol::Buffer req;
    req.write_uint8(static_cast<uint8_t>(protocol::MsgType::MD_COL_DISPLAY));
    req.write_uint64(id_);
    req.write_uint64(static_cast<uint64_t>(column));
    auto resp = conn_->channel()->rpc(req);
    auto status = static_cast<protocol::Status>(resp.read_uint8());
    if (status == protocol::Status::ERROR) {
        throw std::runtime_error(resp.read_string());
    }
    return static_cast<size_t>(resp.read_uint64());
}
