#include <gtest/gtest.h>
#include "query/scan.hpp"
#include "query/constant.hpp"
#include <memory>

// ============================================================================
// Mock concrete Scan implementation for testing
// ============================================================================

class MockScan : public Scan {
public:
  MockScan() : current_position_(-1), before_first_called_(false),
               next_called_(false), close_called_(false) {
    // Initialize test data
    int_data_["id"] = {1, 2, 3};
    string_data_["name"] = {"Alice", "Bob", "Charlie"};
  }

  DbResult<void> before_first() override {
    before_first_called_ = true;
    current_position_ = -1;
    return DbResult<void>::ok();
  }

  DbResult<bool> next() override {
    next_called_ = true;
    current_position_++;
    return DbResult<bool>::ok(current_position_ < 3);
  }

  DbResult<int> get_int(const std::string& fldname) override {
    if (int_data_.count(fldname) && current_position_ >= 0
        && current_position_ < static_cast<int>(int_data_[fldname].size())) {
      return DbResult<int>::ok(int_data_[fldname][current_position_]);
    }
    return DbResult<int>::ok(0);
  }

  DbResult<std::string> get_string(const std::string& fldname) override {
    if (string_data_.count(fldname) && current_position_ >= 0
        && current_position_ < static_cast<int>(string_data_[fldname].size())) {
      return DbResult<std::string>::ok(string_data_[fldname][current_position_]);
    }
    return DbResult<std::string>::ok("");
  }

  DbResult<Constant> get_val(const std::string& fldname) override {
    if (int_data_.count(fldname)) {
      return DbResult<Constant>::ok(Constant::with_int(get_int(fldname).value()));
    }
    if (string_data_.count(fldname)) {
      return DbResult<Constant>::ok(Constant::with_string(get_string(fldname).value()));
    }
    return DbResult<Constant>::ok(Constant::with_int(0));
  }

  bool has_field(const std::string& fldname) const override {
    return int_data_.count(fldname) > 0 || string_data_.count(fldname) > 0;
  }

  DbResult<void> close() override {
    close_called_ = true;
    return DbResult<void>::ok();
  }

  // Test helper methods
  bool was_before_first_called() const { return before_first_called_; }
  bool was_next_called() const { return next_called_; }
  bool was_close_called() const { return close_called_; }

private:
  int current_position_;
  bool before_first_called_;
  bool next_called_;
  bool close_called_;
  std::unordered_map<std::string, std::vector<int>> int_data_;
  std::unordered_map<std::string, std::vector<std::string>> string_data_;
};

// ============================================================================
// Test Scan interface
// ============================================================================

TEST(Scan, PolymorphicUsage) {
  std::unique_ptr<Scan> scan = std::make_unique<MockScan>();
  EXPECT_NE(scan, nullptr);
}

TEST(Scan, BeforeFirst) {
  auto mock_scan = std::make_unique<MockScan>();
  mock_scan->before_first().value();
  EXPECT_TRUE(mock_scan->was_before_first_called());
}

TEST(Scan, NextIteration) {
  auto mock_scan = std::make_unique<MockScan>();
  mock_scan->before_first().value();

  int count = 0;
  while (mock_scan->next().value()) {
    count++;
  }
  EXPECT_EQ(count, 3);
  EXPECT_TRUE(mock_scan->was_next_called());
}

TEST(Scan, GetInt) {
  auto mock_scan = std::make_unique<MockScan>();
  mock_scan->before_first().value();

  EXPECT_TRUE(mock_scan->next().value());
  EXPECT_EQ(mock_scan->get_int("id").value(), 1);

  EXPECT_TRUE(mock_scan->next().value());
  EXPECT_EQ(mock_scan->get_int("id").value(), 2);

  EXPECT_TRUE(mock_scan->next().value());
  EXPECT_EQ(mock_scan->get_int("id").value(), 3);
}

TEST(Scan, GetString) {
  auto mock_scan = std::make_unique<MockScan>();
  mock_scan->before_first().value();

  EXPECT_TRUE(mock_scan->next().value());
  EXPECT_EQ(mock_scan->get_string("name").value(), "Alice");

  EXPECT_TRUE(mock_scan->next().value());
  EXPECT_EQ(mock_scan->get_string("name").value(), "Bob");

  EXPECT_TRUE(mock_scan->next().value());
  EXPECT_EQ(mock_scan->get_string("name").value(), "Charlie");
}

TEST(Scan, GetVal) {
  auto mock_scan = std::make_unique<MockScan>();
  mock_scan->before_first().value();

  EXPECT_TRUE(mock_scan->next().value());
  auto val_int = mock_scan->get_val("id").value();
  EXPECT_TRUE(val_int.as_int().has_value());
  EXPECT_EQ(val_int.as_int().value(), 1);

  auto val_str = mock_scan->get_val("name").value();
  EXPECT_TRUE(val_str.as_string().has_value());
  EXPECT_EQ(val_str.as_string().value(), "Alice");
}

TEST(Scan, HasField) {
  auto mock_scan = std::make_unique<MockScan>();

  EXPECT_TRUE(mock_scan->has_field("id"));
  EXPECT_TRUE(mock_scan->has_field("name"));
  EXPECT_FALSE(mock_scan->has_field("nonexistent"));
}

TEST(Scan, Close) {
  auto mock_scan = std::make_unique<MockScan>();
  mock_scan->close().value();
  EXPECT_TRUE(mock_scan->was_close_called());
}

TEST(Scan, FullScanLifecycle) {
  std::unique_ptr<Scan> scan = std::make_unique<MockScan>();

  // Initialize scan
  scan->before_first().value();

  // Iterate through all records
  int record_count = 0;
  while (scan->next().value()) {
    // Verify we can read fields
    int id = scan->get_int("id").value();
    std::string name = scan->get_string("name").value();

    EXPECT_GT(id, 0);
    EXPECT_FALSE(name.empty());
    record_count++;
  }

  EXPECT_EQ(record_count, 3);

  // Close scan
  scan->close().value();
}

TEST(Scan, SharedPtrUsage) {
  std::shared_ptr<Scan> scan = std::make_shared<MockScan>();
  std::shared_ptr<Scan> scan2 = scan;

  EXPECT_EQ(scan.use_count(), 2);
  scan->before_first().value();
  EXPECT_TRUE(scan->next().value());
}
