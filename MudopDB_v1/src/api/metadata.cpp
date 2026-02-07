#include "api/metadata.hpp"
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
// NetworkMetadata (stub)
// ============================================================================

NetworkMetadata::NetworkMetadata(std::shared_ptr<NetworkConnection> conn, uint64_t id)
    : conn_(std::move(conn)), id_(id) {}

size_t NetworkMetadata::get_column_count() const { return 0; }

std::string NetworkMetadata::get_column_name(size_t) const { return {}; }

record::Type NetworkMetadata::get_column_type(size_t) const { return record::Type::VARCHAR; }

size_t NetworkMetadata::get_column_display_size(size_t) const { return 10; }
