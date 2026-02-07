#ifndef METADATA_HPP
#define METADATA_HPP

#include "record/schema.hpp"
#include <cstddef>
#include <memory>
#include <string>

class NetworkConnection;

/**
 * Abstract base class for result set metadata.
 * Corresponds to MetaDataControl trait in Rust (NMDB2/src/api/metadata.rs)
 */
class Metadata {
public:
    virtual ~Metadata() = default;
    virtual size_t get_column_count() const = 0;
    virtual std::string get_column_name(size_t column) const = 0;
    virtual record::Type get_column_type(size_t column) const = 0;
    virtual size_t get_column_display_size(size_t column) const = 0;
};

/**
 * Embedded metadata backed by a Schema.
 * Corresponds to EmbeddedMetaData in Rust (NMDB2/src/api/embedded/embeddedmetadata.rs)
 */
class EmbeddedMetadata : public Metadata {
public:
    explicit EmbeddedMetadata(std::shared_ptr<record::Schema> schema);

    size_t get_column_count() const override;
    std::string get_column_name(size_t column) const override;
    record::Type get_column_type(size_t column) const override;
    size_t get_column_display_size(size_t column) const override;

private:
    std::shared_ptr<record::Schema> sch_;
};

/**
 * Network metadata (stub).
 */
class NetworkMetadata : public Metadata {
public:
    NetworkMetadata(std::shared_ptr<NetworkConnection> conn, uint64_t id);

    size_t get_column_count() const override;
    std::string get_column_name(size_t column) const override;
    record::Type get_column_type(size_t column) const override;
    size_t get_column_display_size(size_t column) const override;

private:
    std::shared_ptr<NetworkConnection> conn_;
    uint64_t id_;
};

#endif // METADATA_HPP
