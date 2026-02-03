#include "api/metadata.hpp"
#include <memory>
#include <string>

EmbeddedMetadata::EmbeddedMetadata(std::shared_ptr<record::Schema> schema)
	: sch_(std::move(schema)) {}

EmbeddedMetadata::~EmbeddedMetadata() = default;

size_t EmbeddedMetadata::get_column_count() const {
	return sch_ ? sch_->fields().size() : 0;
}

std::string EmbeddedMetadata::get_column_name(size_t column) const {
	if (!sch_ || column == 0 || column > sch_->fields().size()) return {};
	return sch_->fields()[column - 1];
}

record::Type EmbeddedMetadata::get_column_type(size_t /*column*/) const {
	return record::Type::INTEGER;
}

size_t EmbeddedMetadata::get_column_display_size(size_t /*column*/) const {
	return 10;
}

NetworkMetadata::NetworkMetadata(std::shared_ptr<NetworkConnection> conn, std::uint64_t id)
	: conn_(std::move(conn)), id_(id) {}

NetworkMetadata::~NetworkMetadata() = default;

size_t NetworkMetadata::get_column_count() const { return 0; }

std::string NetworkMetadata::get_column_name(size_t /*column*/) const { return {}; }

record::Type NetworkMetadata::get_column_type(size_t /*column*/) const { return record::Type::VARCHAR; }

size_t NetworkMetadata::get_column_display_size(size_t /*column*/) const { return 10; }
