#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <filesystem>
#include "file/memfilemgr.hpp"
#include "file/page.hpp"
#include "file/blockid.hpp"
#include "server/simpledb.hpp"
#include "plan/plan.hpp"
#include "query/scan.hpp"
#include "api/driver.hpp"
#include "api/connection.hpp"
#include "api/statement.hpp"
#include "api/result_set.hpp"
#include "api/metadata.hpp"

// ============================================================================
// MemFileMgr unit tests
// ============================================================================

TEST(MemFileMgr, AppendAndLength) {
    auto fm = std::make_shared<file::MemFileMgr>(400);
    EXPECT_EQ(fm->length("test.tbl"), 0u);

    auto blk0 = fm->append("test.tbl");
    EXPECT_EQ(blk0.number(), 0);
    EXPECT_EQ(fm->length("test.tbl"), 1u);

    auto blk1 = fm->append("test.tbl");
    EXPECT_EQ(blk1.number(), 1);
    EXPECT_EQ(fm->length("test.tbl"), 2u);
}

TEST(MemFileMgr, WriteAndRead) {
    auto fm = std::make_shared<file::MemFileMgr>(400);

    // Write data to a block
    fm->append("test.tbl");
    file::Page wp(400);
    wp.set_int(0, 42);
    wp.set_string(4, "hello");
    file::BlockId blk("test.tbl", 0);
    fm->write(blk, wp);

    // Read it back
    file::Page rp(400);
    fm->read(blk, rp);
    EXPECT_EQ(rp.get_int(0), 42);
    EXPECT_EQ(rp.get_string(4), "hello");
}

TEST(MemFileMgr, ReadNonexistentFileReturnsZeros) {
    auto fm = std::make_shared<file::MemFileMgr>(400);
    file::BlockId blk("nofile.tbl", 0);
    file::Page page(400);
    fm->read(blk, page);
    EXPECT_EQ(page.get_int(0), 0);
}

TEST(MemFileMgr, ReadBeyondEndReturnsZeros) {
    auto fm = std::make_shared<file::MemFileMgr>(400);
    fm->append("test.tbl");  // block 0 exists
    file::BlockId blk("test.tbl", 5);  // block 5 does not
    file::Page page(400);
    fm->read(blk, page);
    EXPECT_EQ(page.get_int(0), 0);
}

TEST(MemFileMgr, MultipleFilesAreIsolated) {
    auto fm = std::make_shared<file::MemFileMgr>(400);

    fm->append("a.tbl");
    fm->append("b.tbl");

    file::Page wp(400);
    wp.set_int(0, 100);
    fm->write(file::BlockId("a.tbl", 0), wp);

    wp.set_int(0, 200);
    fm->write(file::BlockId("b.tbl", 0), wp);

    file::Page rp(400);
    fm->read(file::BlockId("a.tbl", 0), rp);
    EXPECT_EQ(rp.get_int(0), 100);

    fm->read(file::BlockId("b.tbl", 0), rp);
    EXPECT_EQ(rp.get_int(0), 200);
}

TEST(MemFileMgr, IsNewReturnsTrue) {
    auto fm = std::make_shared<file::MemFileMgr>(400);
    EXPECT_TRUE(fm->is_new());
}

TEST(MemFileMgr, BlockSizeIsCorrect) {
    auto fm = std::make_shared<file::MemFileMgr>(512);
    EXPECT_EQ(fm->block_size(), 512u);
}

// ============================================================================
// SimpleDB::in_memory() integration tests
// Note: Scan::next() returns DbResult<bool> - must use .value() to get bool
// ============================================================================

TEST(InMemoryDB, CreatesSuccessfully) {
    auto db = server::SimpleDB::in_memory();
    EXPECT_NE(db.file_mgr(), nullptr);
    EXPECT_NE(db.log_mgr(), nullptr);
    EXPECT_NE(db.buffer_mgr(), nullptr);
    EXPECT_NE(db.md_mgr(), nullptr);
    EXPECT_NE(db.planner(), nullptr);
}

TEST(InMemoryDB, CreateTableAndInsert) {
    auto db = server::SimpleDB::in_memory();
    auto tx = db.new_tx();
    auto planner = db.planner();

    planner->execute_update("create table test1 (id int, name varchar(20))", tx);
    planner->execute_update("insert into test1 (id, name) values (1, 'alice')", tx);
    planner->execute_update("insert into test1 (id, name) values (2, 'bob')", tx);

    auto plan = planner->create_query_plan("select id, name from test1", tx);
    auto scan = plan->open();

    int count = 0;
    while (scan->next().value()) {
        count++;
        int id = scan->get_int("id").value();
        EXPECT_TRUE(id == 1 || id == 2);
    }
    EXPECT_EQ(count, 2);

    scan->close().value();
    tx->commit();
}

TEST(InMemoryDB, MultipleTransactions) {
    auto db = server::SimpleDB::in_memory();

    // Transaction 1: create and insert
    {
        auto tx = db.new_tx();
        db.planner()->execute_update("create table t2 (val int)", tx);
        db.planner()->execute_update("insert into t2 (val) values (10)", tx);
        tx->commit();
    }

    // Transaction 2: read back
    {
        auto tx = db.new_tx();
        auto plan = db.planner()->create_query_plan("select val from t2", tx);
        auto scan = plan->open();
        ASSERT_TRUE(scan->next().value());
        EXPECT_EQ(scan->get_int("val").value(), 10);
        EXPECT_FALSE(scan->next().value());
        scan->close().value();
        tx->commit();
    }
}

TEST(InMemoryDB, DeleteRows) {
    auto db = server::SimpleDB::in_memory();
    auto tx = db.new_tx();
    auto planner = db.planner();

    planner->execute_update("create table t3 (x int)", tx);
    planner->execute_update("insert into t3 (x) values (1)", tx);
    planner->execute_update("insert into t3 (x) values (2)", tx);
    planner->execute_update("insert into t3 (x) values (3)", tx);

    size_t deleted = planner->execute_update("delete from t3 where x = 2", tx);
    EXPECT_EQ(deleted, 1u);

    auto plan = planner->create_query_plan("select x from t3", tx);
    auto scan = plan->open();
    int count = 0;
    while (scan->next().value()) {
        EXPECT_NE(scan->get_int("x").value(), 2);
        count++;
    }
    EXPECT_EQ(count, 2);
    scan->close().value();
    tx->commit();
}

TEST(InMemoryDB, UpdateRows) {
    auto db = server::SimpleDB::in_memory();
    auto tx = db.new_tx();
    auto planner = db.planner();

    planner->execute_update("create table t4 (a int, b varchar(10))", tx);
    planner->execute_update("insert into t4 (a, b) values (1, 'old')", tx);

    size_t updated = planner->execute_update("update t4 set b = 'new' where a = 1", tx);
    EXPECT_EQ(updated, 1u);

    auto plan = planner->create_query_plan("select b from t4 where a = 1", tx);
    auto scan = plan->open();
    ASSERT_TRUE(scan->next().value());
    EXPECT_EQ(scan->get_string("b").value(), "new");
    scan->close().value();
    tx->commit();
}

TEST(InMemoryDB, RollbackUndoesChanges) {
    auto db = server::SimpleDB::in_memory();

    // Create table and insert in tx1
    {
        auto tx = db.new_tx();
        db.planner()->execute_update("create table t5 (v int)", tx);
        db.planner()->execute_update("insert into t5 (v) values (100)", tx);
        tx->commit();
    }

    // Insert in tx2 then rollback
    {
        auto tx = db.new_tx();
        db.planner()->execute_update("insert into t5 (v) values (200)", tx);
        tx->rollback();
    }

    // Verify only original row remains
    {
        auto tx = db.new_tx();
        auto plan = db.planner()->create_query_plan("select v from t5", tx);
        auto scan = plan->open();
        int count = 0;
        while (scan->next().value()) {
            EXPECT_EQ(scan->get_int("v").value(), 100);
            count++;
        }
        EXPECT_EQ(count, 1);
        scan->close().value();
        tx->commit();
    }
}

TEST(InMemoryDB, CreateIndex) {
    auto db = server::SimpleDB::in_memory();
    auto tx = db.new_tx();
    auto planner = db.planner();

    planner->execute_update("create table t6 (id int, name varchar(20))", tx);
    planner->execute_update("create index idx_t6_id on t6 (id)", tx);
    planner->execute_update("insert into t6 (id, name) values (10, 'ten')", tx);
    planner->execute_update("insert into t6 (id, name) values (20, 'twenty')", tx);

    auto plan = planner->create_query_plan("select name from t6 where id = 10", tx);
    auto scan = plan->open();
    ASSERT_TRUE(scan->next().value());
    EXPECT_EQ(scan->get_string("name").value(), "ten");
    scan->close().value();
    tx->commit();
}

TEST(InMemoryDB, LargerDataSet) {
    auto db = server::SimpleDB::in_memory();
    auto tx = db.new_tx();
    auto planner = db.planner();

    planner->execute_update("create table t7 (num int)", tx);

    for (int i = 0; i < 50; i++) {
        std::string sql = "insert into t7 (num) values (" + std::to_string(i) + ")";
        planner->execute_update(sql, tx);
    }

    auto plan = planner->create_query_plan("select num from t7", tx);
    auto scan = plan->open();
    int count = 0;
    while (scan->next().value()) {
        count++;
    }
    EXPECT_EQ(count, 50);
    scan->close().value();
    tx->commit();
}

// ============================================================================
// EmbeddedDriver "mem:" connection string tests
// Note: ResultSet::next() returns plain bool (no .value() needed)
// ============================================================================

TEST(InMemoryDriver, ConnectWithMemUrl) {
    EmbeddedDriver d;
    auto conn = d.connect("mem:");
    ASSERT_NE(conn, nullptr);
    auto stmt = conn->create_statement();
    EXPECT_NE(stmt, nullptr);
    conn->close();
}

TEST(InMemoryDriver, ConnectWithMemoryUrl) {
    EmbeddedDriver d;
    auto conn = d.connect(":memory:");
    ASSERT_NE(conn, nullptr);
    auto stmt = conn->create_statement();
    EXPECT_NE(stmt, nullptr);
    conn->close();
}

TEST(InMemoryDriver, FullCRUDThroughDriver) {
    EmbeddedDriver d;
    auto conn = d.connect("mem:");
    auto stmt = conn->create_statement();

    stmt->execute_update("create table people (age int, name varchar(20))");
    stmt->execute_update("insert into people (age, name) values (30, 'alice')");
    stmt->execute_update("insert into people (age, name) values (25, 'bob')");

    auto rs = stmt->execute_query("select age, name from people");
    auto meta = rs->get_meta_data();
    EXPECT_EQ(meta->get_column_count(), 2u);

    int count = 0;
    while (rs->next()) {
        count++;
        int age = rs->get_int("age");
        std::string name = rs->get_string("name");
        EXPECT_TRUE((age == 30 && name == "alice") || (age == 25 && name == "bob"));
    }
    EXPECT_EQ(count, 2);
    rs->close();
    conn->close();
}

TEST(InMemoryDriver, NoFilesOnDisk) {
    EmbeddedDriver d;
    auto conn = d.connect("mem:");
    auto stmt = conn->create_statement();
    stmt->execute_update("create table ephemeral (x int)");
    stmt->execute_update("insert into ephemeral (x) values (1)");

    auto rs = stmt->execute_query("select x from ephemeral");
    ASSERT_TRUE(rs->next());
    EXPECT_EQ(rs->get_int("x"), 1);
    rs->close();
    conn->close();

    // No directory should have been created for "mem:"
    EXPECT_FALSE(std::filesystem::exists("mem:"));
}
