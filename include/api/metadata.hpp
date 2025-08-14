#ifndef METADATA_HPP
#define METADATA_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include "record/schema.hpp"

class Connection;
class NetworkConnection;

class Metadata {
public:
  virtual ~Metadata() = default;
  virtual size_t get_column_count() const = 0;
  virtual std::string get_column_name(size_t column) const = 0;
  virtual record::Type get_column_type(size_t column) const = 0;
  virtual size_t get_column_display_size(size_t column) const = 0;
};

class EmbeddedMetadata : public Metadata {
public:
  explicit EmbeddedMetadata(std::shared_ptr<record::Schema> schema);
  ~EmbeddedMetadata() override;

  size_t get_column_count() const override;
  std::string get_column_name(size_t column) const override;
  record::Type get_column_type(size_t column) const override;
  size_t get_column_display_size(size_t column) const override; 
private:
  std::shared_ptr<record::Schema> sch_;
};
class NetworkMetadata : public Metadata {
public:
  explicit NetworkMetadata(std::shared_ptr<NetworkConnection> conn, std::uint64_t id);
  ~NetworkMetadata() override;

  size_t get_column_count() const override;
  std::string get_column_name(size_t column) const override;
  record::Type get_column_type(size_t column) const override;
  size_t get_column_display_size(size_t column) const override;
private:
  std::shared_ptr<NetworkConnection> conn_;
  //std::unique_ptr<simpledb::MetaDataClient> client_;
  std::uint64_t id_;
};

#endif // METADATA_HPP