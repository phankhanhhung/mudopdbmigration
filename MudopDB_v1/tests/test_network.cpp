#include <gtest/gtest.h>
#include "server/simpledb_server.hpp"
#include "server/tcp_transport.hpp"
#include "api/connection.hpp"
#include "api/statement.hpp"
#include "api/result_set.hpp"
#include "api/metadata.hpp"
#include "api/driver.hpp"
#include <memory>
#include <thread>
#include <chrono>
#include <cstdlib>

static int net_test_counter = 0;

// Helper: start a TcpServer in a background thread, return the port
class NetworkTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a unique DB directory per test to avoid stale data
        db_name_ = "network_testdb_" + std::to_string(net_test_counter++);
        std::system(("rm -rf " + db_name_).c_str());

        // Create server with port 0 (OS assigns free port)
        db_server_ = std::make_unique<server::SimpleDBServer>(db_name_);
        tcp_server_ = std::make_unique<transport::TcpServer>(*db_server_, 0);

        // Start server in background thread
        server_thread_ = std::thread([this]() {
            tcp_server_->serve();
        });

        // Wait for server to start listening
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        port_ = tcp_server_->port();
    }

    void TearDown() override {
        tcp_server_->stop();
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        // Clean up DB directory
        db_server_.reset();
        std::system(("rm -rf " + db_name_).c_str());
    }

    std::string db_name_;
    uint16_t port_ = 0;
    std::unique_ptr<server::SimpleDBServer> db_server_;
    std::unique_ptr<transport::TcpServer> tcp_server_;
    std::thread server_thread_;
};

// ============================================================================
// Protocol buffer encoding/decoding tests
// ============================================================================

TEST(ProtocolBuffer, EncodeDecodeUint8) {
    protocol::Buffer buf;
    buf.write_uint8(42);
    buf.reset_read();
    EXPECT_EQ(buf.read_uint8(), 42);
}

TEST(ProtocolBuffer, EncodeDecodeUint32) {
    protocol::Buffer buf;
    buf.write_uint32(123456);
    buf.reset_read();
    EXPECT_EQ(buf.read_uint32(), 123456u);
}

TEST(ProtocolBuffer, EncodeDecodeInt32) {
    protocol::Buffer buf;
    buf.write_int32(-42);
    buf.reset_read();
    EXPECT_EQ(buf.read_int32(), -42);
}

TEST(ProtocolBuffer, EncodeDecodeUint64) {
    protocol::Buffer buf;
    buf.write_uint64(9876543210ULL);
    buf.reset_read();
    EXPECT_EQ(buf.read_uint64(), 9876543210ULL);
}

TEST(ProtocolBuffer, EncodeDecodeBool) {
    protocol::Buffer buf;
    buf.write_bool(true);
    buf.write_bool(false);
    buf.reset_read();
    EXPECT_TRUE(buf.read_bool());
    EXPECT_FALSE(buf.read_bool());
}

TEST(ProtocolBuffer, EncodeDecodeString) {
    protocol::Buffer buf;
    buf.write_string("hello world");
    buf.reset_read();
    EXPECT_EQ(buf.read_string(), "hello world");
}

TEST(ProtocolBuffer, EncodeDecodeEmptyString) {
    protocol::Buffer buf;
    buf.write_string("");
    buf.reset_read();
    EXPECT_EQ(buf.read_string(), "");
}

TEST(ProtocolBuffer, ReadPastEndThrows) {
    protocol::Buffer buf;
    buf.write_uint8(1);
    buf.reset_read();
    buf.read_uint8();  // ok
    EXPECT_THROW(buf.read_uint8(), std::runtime_error);
}

TEST(ProtocolBuffer, MultipleValuesRoundTrip) {
    protocol::Buffer buf;
    buf.write_uint8(0x11);
    buf.write_string("select * from t");
    buf.write_uint64(42);
    buf.write_int32(-1);
    buf.write_bool(true);

    buf.reset_read();
    EXPECT_EQ(buf.read_uint8(), 0x11);
    EXPECT_EQ(buf.read_string(), "select * from t");
    EXPECT_EQ(buf.read_uint64(), 42u);
    EXPECT_EQ(buf.read_int32(), -1);
    EXPECT_TRUE(buf.read_bool());
}

// ============================================================================
// Network integration tests (server + client)
// ============================================================================

TEST_F(NetworkTestFixture, ConnectAndCommit) {
    NetworkConnection conn("127.0.0.1", port_);
    EXPECT_NO_THROW(conn.commit());
}

TEST_F(NetworkTestFixture, ConnectAndRollback) {
    NetworkConnection conn("127.0.0.1", port_);
    EXPECT_NO_THROW(conn.rollback());
}

TEST_F(NetworkTestFixture, CreateTableInsertAndQuery) {
    NetworkConnection conn("127.0.0.1", port_);
    auto stmt = conn.create_statement();

    // Create table
    size_t count = stmt->execute_update("create table test1 (id int, name varchar(20))");
    EXPECT_EQ(count, 0u);

    // Insert rows
    count = stmt->execute_update("insert into test1 (id, name) values (1, 'alice')");
    EXPECT_EQ(count, 1u);
    count = stmt->execute_update("insert into test1 (id, name) values (2, 'bob')");
    EXPECT_EQ(count, 1u);

    // Query
    auto rs = stmt->execute_query("select id, name from test1");
    ASSERT_NE(rs, nullptr);

    // Check metadata
    auto md = rs->get_meta_data();
    EXPECT_EQ(md->get_column_count(), 2u);
    EXPECT_EQ(md->get_column_name(1), "id");
    EXPECT_EQ(md->get_column_name(2), "name");

    // Read rows
    int row_count = 0;
    while (rs->next()) {
        int32_t id = rs->get_int("id");
        std::string name = rs->get_string("name");
        EXPECT_TRUE(id == 1 || id == 2);
        if (id == 1) EXPECT_EQ(name, "alice");
        if (id == 2) EXPECT_EQ(name, "bob");
        row_count++;
    }
    EXPECT_EQ(row_count, 2);

    rs->close();
}

TEST_F(NetworkTestFixture, ExecuteUpdateReturnsRowCount) {
    NetworkConnection conn("127.0.0.1", port_);
    auto stmt = conn.create_statement();

    stmt->execute_update("create table test2 (val int)");
    stmt->execute_update("insert into test2 (val) values (10)");
    stmt->execute_update("insert into test2 (val) values (20)");
    stmt->execute_update("insert into test2 (val) values (30)");

    size_t deleted = stmt->execute_update("delete from test2 where val = 20");
    EXPECT_EQ(deleted, 1u);
}

TEST_F(NetworkTestFixture, NetworkDriverConnect) {
    NetworkDriver driver;
    std::string url = "localhost:" + std::to_string(port_);
    auto conn = driver.connect(url);
    ASSERT_NE(conn, nullptr);
    EXPECT_NO_THROW(conn->commit());
    EXPECT_NO_THROW(conn->close());
}

TEST_F(NetworkTestFixture, CloseConnection) {
    NetworkConnection conn("127.0.0.1", port_);
    EXPECT_NO_THROW(conn.close());
}
