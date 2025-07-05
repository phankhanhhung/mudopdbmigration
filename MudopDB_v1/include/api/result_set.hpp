#ifndef RESULT_SET_HPP
#define RESULT_SET_HPP

#include <cstdint>
#include <string>
#include <memory>
#include "plan/plan.hpp"
#include "query/scan.hpp"
#include "record/schema.hpp"

class Metadata;
class Connection;
class NetworkConnection;
class EmbeddedConnection;

class ResultSet {
public:
  virtual ~ResultSet() = default;
  virtual bool next() = 0;
  virtual std::int32_t get_int(std::string fldname) = 0;
  virtual std::string get_string(std::string fldname) = 0;
  virtual const Metadata* get_meta_data() const noexcept = 0;
  virtual void close() = 0;
};

class EmbeddedResultSet : public ResultSet {
public:
  explicit EmbeddedResultSet(std::shared_ptr<Plan> plan, std::shared_ptr<EmbeddedConnection>);
  bool next() override;
  std::int32_t get_int(std::string fldname) override;
  std::string get_string(std::string fldname) override;
  const Metadata* get_meta_data() const noexcept override;
  void close() override;
private:
  std::shared_ptr<Scan> s;
  std::shared_ptr<record::Schema> sch;
  std::shared_ptr<EmbeddedConnection> conn;
};
class NetworkResultSet : public ResultSet {
public:
  explicit NetworkResultSet(std::shared_ptr<NetworkConnection> conn, int64_t id);
  bool next() override;
  std::int32_t get_int(std::string fldname) override;
  std::string get_string(std::string fldname) override;
  const Metadata* get_meta_data() const noexcept override;
  void close() override;
};


#endif // RESULT_SET_HPP