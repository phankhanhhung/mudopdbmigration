#include <gtest/gtest.h>
#include <sstream>
#include <functional>
#include <string>
#include <memory>
#include "helper/query_update.hpp"
#include "api/statement.hpp"
#include "api/metadata.hpp"
#include "api/result_set.hpp"

// Helper to capture std::cout during the call
static std::string capture(std::function<void()> fn) {
  std::ostringstream oss;
  auto* oldbuf = std::cout.rdbuf(oss.rdbuf());
  fn();
  std::cout.rdbuf(oldbuf);
  return oss.str();
}

// ============================================================================
// Mock Metadata with configurable fields
// ============================================================================
class MockMetadata : public Metadata {
public:
  struct ColumnDef {
    std::string name;
    record::Type type;
    size_t display_size;
  };

  explicit MockMetadata(std::vector<ColumnDef> columns) : columns_(std::move(columns)) {}
  virtual ~MockMetadata() = default;

  size_t get_column_count() const override {
    return columns_.size();
  }

  std::string get_column_name(size_t column) const override {
    return (column >= 1 && column <= columns_.size()) ? columns_[column - 1].name : "";
  }

  record::Type get_column_type(size_t column) const override {
    return (column >= 1 && column <= columns_.size()) ? columns_[column - 1].type : record::Type::INTEGER;
  }

  size_t get_column_display_size(size_t column) const override {
    return (column >= 1 && column <= columns_.size()) ? columns_[column - 1].display_size : 0;
  }

private:
  std::vector<ColumnDef> columns_;
};

// ============================================================================
// Mock ResultSet with configurable rows
// ============================================================================
class MockResultSet : public ResultSet {
public:
  struct Row {
    std::unordered_map<std::string, std::int32_t> int_values;
    std::unordered_map<std::string, std::string> string_values;
  };

  MockResultSet(std::vector<Row> rows, std::unique_ptr<Metadata> metadata)
    : rows_(std::move(rows)), metadata_(std::move(metadata)), current_row_(-1), closed_(false) {}

  virtual ~MockResultSet() = default;

  bool next() override {
    if (closed_ || current_row_ + 1 >= static_cast<int>(rows_.size())) {
      return false;
    }
    current_row_++;
    return true;
  }

  std::int32_t get_int(std::string fldname) override {
    if (current_row_ < 0 || current_row_ >= static_cast<int>(rows_.size())) {
      return 0;
    }
    auto it = rows_[current_row_].int_values.find(fldname);
    return (it != rows_[current_row_].int_values.end()) ? it->second : 0;
  }

  std::string get_string(std::string fldname) override {
    if (current_row_ < 0 || current_row_ >= static_cast<int>(rows_.size())) {
      return "";
    }
    auto it = rows_[current_row_].string_values.find(fldname);
    return (it != rows_[current_row_].string_values.end()) ? it->second : "";
  }

  const Metadata* get_meta_data() const noexcept override {
    return metadata_.get();
  }

  void close() override {
    closed_ = true;
  }

  bool is_closed() const { return closed_; }

private:
  std::vector<Row> rows_;
  std::unique_ptr<Metadata> metadata_;
  int current_row_;
  bool closed_;
};

// ============================================================================
// Mock Statement
// ============================================================================
class MockStatement : public Statement {
public:
  MockStatement() : last_query_cmd_(""), last_update_cmd_(""), update_count_(0) {}

  void set_query_result(std::unique_ptr<ResultSet> rs) {
    query_result_ = std::move(rs);
  }

  void set_update_count(size_t count) {
    update_count_ = count;
  }

  std::unique_ptr<ResultSet> execute_query(std::string qry) override {
    last_query_cmd_ = qry;
    return std::move(query_result_);
  }

  size_t execute_update(std::string cmd) override {
    last_update_cmd_ = cmd;
    return update_count_;
  }

  std::string get_last_query_cmd() const { return last_query_cmd_; }
  std::string get_last_update_cmd() const { return last_update_cmd_; }

private:
  std::unique_ptr<ResultSet> query_result_;
  std::string last_query_cmd_;
  std::string last_update_cmd_;
  size_t update_count_;
};

// ============================================================================
// Tests for do_query
// ============================================================================

TEST(DoQueryTest, ExecutesQueryWithCorrectCommand) {
  auto stmt = std::make_unique<MockStatement>();
  auto metadata = std::make_unique<MockMetadata>(std::vector<MockMetadata::ColumnDef>{});
  auto rs = std::make_unique<MockResultSet>(std::vector<MockResultSet::Row>{}, std::move(metadata));

  stmt->set_query_result(std::move(rs));

  auto out = capture([&]{ do_query(stmt.get(), "SELECT * FROM users"); });

  EXPECT_NE(out.find("Executing query: SELECT * FROM users"), std::string::npos);
  EXPECT_EQ(stmt->get_last_query_cmd(), "SELECT * FROM users");
}

TEST(DoQueryTest, HandlesEmptyResultSet) {
  auto stmt = std::make_unique<MockStatement>();
  auto metadata = std::make_unique<MockMetadata>(std::vector<MockMetadata::ColumnDef>{
    {"id", record::Type::INTEGER, 10},
    {"name", record::Type::VARCHAR, 20}
  });
  auto rs = std::make_unique<MockResultSet>(std::vector<MockResultSet::Row>{}, std::move(metadata));

  stmt->set_query_result(std::move(rs));

  auto out = capture([&]{ do_query(stmt.get(), "SELECT * FROM empty_table"); });

  EXPECT_NE(out.find("Executing query: SELECT * FROM empty_table"), std::string::npos);
}

TEST(DoQueryTest, DisplaysIntegerColumn) {
  auto stmt = std::make_unique<MockStatement>();
  auto metadata = std::make_unique<MockMetadata>(std::vector<MockMetadata::ColumnDef>{
    {"id", record::Type::INTEGER, 10}
  });

  MockResultSet::Row row1;
  row1.int_values["id"] = 42;

  MockResultSet::Row row2;
  row2.int_values["id"] = 99;

  auto rs = std::make_unique<MockResultSet>(
    std::vector<MockResultSet::Row>{row1, row2},
    std::move(metadata)
  );

  stmt->set_query_result(std::move(rs));

  auto out = capture([&]{ do_query(stmt.get(), "SELECT id FROM table1"); });

  EXPECT_NE(out.find("42"), std::string::npos);
  EXPECT_NE(out.find("99"), std::string::npos);
}

TEST(DoQueryTest, DisplaysVarcharColumn) {
  auto stmt = std::make_unique<MockStatement>();
  auto metadata = std::make_unique<MockMetadata>(std::vector<MockMetadata::ColumnDef>{
    {"name", record::Type::VARCHAR, 20}
  });

  MockResultSet::Row row1;
  row1.string_values["name"] = "Alice";

  MockResultSet::Row row2;
  row2.string_values["name"] = "Bob";

  auto rs = std::make_unique<MockResultSet>(
    std::vector<MockResultSet::Row>{row1, row2},
    std::move(metadata)
  );

  stmt->set_query_result(std::move(rs));

  auto out = capture([&]{ do_query(stmt.get(), "SELECT name FROM users"); });

  EXPECT_NE(out.find("Alice"), std::string::npos);
  EXPECT_NE(out.find("Bob"), std::string::npos);
}

TEST(DoQueryTest, DisplaysMultipleColumns) {
  auto stmt = std::make_unique<MockStatement>();
  auto metadata = std::make_unique<MockMetadata>(std::vector<MockMetadata::ColumnDef>{
    {"id", record::Type::INTEGER, 10},
    {"name", record::Type::VARCHAR, 20},
    {"age", record::Type::INTEGER, 5}
  });

  MockResultSet::Row row1;
  row1.int_values["id"] = 1;
  row1.string_values["name"] = "Alice";
  row1.int_values["age"] = 30;

  MockResultSet::Row row2;
  row2.int_values["id"] = 2;
  row2.string_values["name"] = "Bob";
  row2.int_values["age"] = 25;

  auto rs = std::make_unique<MockResultSet>(
    std::vector<MockResultSet::Row>{row1, row2},
    std::move(metadata)
  );

  stmt->set_query_result(std::move(rs));

  auto out = capture([&]{ do_query(stmt.get(), "SELECT * FROM users"); });

  EXPECT_NE(out.find("1"), std::string::npos);
  EXPECT_NE(out.find("Alice"), std::string::npos);
  EXPECT_NE(out.find("30"), std::string::npos);
  EXPECT_NE(out.find("2"), std::string::npos);
  EXPECT_NE(out.find("Bob"), std::string::npos);
  EXPECT_NE(out.find("25"), std::string::npos);
}

TEST(DoQueryTest, ClosesResultSetAfterCompletion) {
  auto stmt = std::make_unique<MockStatement>();
  auto metadata = std::make_unique<MockMetadata>(std::vector<MockMetadata::ColumnDef>{
    {"id", record::Type::INTEGER, 10}
  });

  MockResultSet::Row row;
  row.int_values["id"] = 1;

  auto rs_ptr = new MockResultSet(std::vector<MockResultSet::Row>{row}, std::move(metadata));
  stmt->set_query_result(std::unique_ptr<ResultSet>(rs_ptr));

  capture([&]{ do_query(stmt.get(), "SELECT id FROM test"); });

  EXPECT_TRUE(rs_ptr->is_closed());
}

// ============================================================================
// Tests for do_update
// ============================================================================

TEST(DoUpdateTest, ExecutesUpdateWithCorrectCommand) {
  auto stmt = std::make_unique<MockStatement>();
  stmt->set_update_count(5);

  auto out = capture([&]{ do_update(stmt.get(), "UPDATE users SET name='test'"); });

  EXPECT_NE(out.find("Executing update: UPDATE users SET name='test'"), std::string::npos);
  EXPECT_EQ(stmt->get_last_update_cmd(), "UPDATE users SET name='test'");
}

TEST(DoUpdateTest, DisplaysCorrectRecordCount) {
  auto stmt = std::make_unique<MockStatement>();
  stmt->set_update_count(3);

  auto out = capture([&]{ do_update(stmt.get(), "DELETE FROM users WHERE id > 10"); });

  EXPECT_NE(out.find("3 records processed"), std::string::npos);
}

TEST(DoUpdateTest, HandlesZeroRecordsUpdated) {
  auto stmt = std::make_unique<MockStatement>();
  stmt->set_update_count(0);

  auto out = capture([&]{ do_update(stmt.get(), "UPDATE users SET status='active' WHERE id=-1"); });

  EXPECT_NE(out.find("0 records processed"), std::string::npos);
}

TEST(DoUpdateTest, HandlesInsertCommand) {
  auto stmt = std::make_unique<MockStatement>();
  stmt->set_update_count(1);

  auto out = capture([&]{ do_update(stmt.get(), "INSERT INTO users (id, name) VALUES (1, 'Alice')"); });

  EXPECT_NE(out.find("Executing update: INSERT INTO users (id, name) VALUES (1, 'Alice')"), std::string::npos);
  EXPECT_NE(out.find("1 records processed"), std::string::npos);
}

TEST(DoUpdateTest, HandlesDeleteCommand) {
  auto stmt = std::make_unique<MockStatement>();
  stmt->set_update_count(10);

  auto out = capture([&]{ do_update(stmt.get(), "DELETE FROM logs WHERE date < '2024-01-01'"); });

  EXPECT_NE(out.find("Executing update: DELETE FROM logs WHERE date < '2024-01-01'"), std::string::npos);
  EXPECT_NE(out.find("10 records processed"), std::string::npos);
}

TEST(DoUpdateTest, HandlesCreateTableCommand) {
  auto stmt = std::make_unique<MockStatement>();
  stmt->set_update_count(0);

  auto out = capture([&]{ do_update(stmt.get(), "CREATE TABLE test (id INT, name VARCHAR(50))"); });

  EXPECT_NE(out.find("Executing update: CREATE TABLE test (id INT, name VARCHAR(50))"), std::string::npos);
  EXPECT_NE(out.find("0 records processed"), std::string::npos);
}

TEST(DoUpdateTest, HandlesLargeUpdateCount) {
  auto stmt = std::make_unique<MockStatement>();
  stmt->set_update_count(1000000);

  auto out = capture([&]{ do_update(stmt.get(), "UPDATE large_table SET processed=true"); });

  EXPECT_NE(out.find("1000000 records processed"), std::string::npos);
}
