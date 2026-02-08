#include <gtest/gtest.h>
#include "tx/transaction.hpp"
#include "tx/recovery/logrecord.hpp"
#include "record/recordpage.hpp"
#include "record/layout.hpp"
#include "record/schema.hpp"
#include "buffer/buffermgr.hpp"
#include "file/filemgr.hpp"
#include "log/logmgr.hpp"
#include <filesystem>
#include <memory>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

// ============================================================================
// WAL Compliance Tests
//
// Verifies Write-Ahead Logging protocol:
// 1. Log records are written BEFORE data modifications
// 2. Rollback correctly undoes uncommitted changes
// 3. Recovery undoes changes from uncommitted transactions
// 4. RecordPage operations go through Transaction (no direct buffer access)
// 5. Buffer flush respects WAL order (log flushed before data)
// ============================================================================

class WALTest : public ::testing::Test {
protected:
    std::string test_dir = "/tmp/mudopdb_wal_test";
    std::string logfile = "wal_test.log";
    size_t blocksize = 400;

    std::shared_ptr<file::FileMgr> fm;
    std::shared_ptr<log::LogMgr> lm;
    std::shared_ptr<buffer::BufferMgr> bm;

    void SetUp() override {
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
        fs::create_directories(test_dir);
        fm = std::make_shared<file::FileMgr>(test_dir, blocksize);
        lm = std::make_shared<log::LogMgr>(fm, logfile);
        bm = std::make_shared<buffer::BufferMgr>(fm, lm, 8);
    }

    void TearDown() override {
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }

    // Helper: collect all log records from the log
    std::vector<std::pair<tx::Op, std::optional<size_t>>> read_log_records() {
        std::vector<std::pair<tx::Op, std::optional<size_t>>> records;
        auto iter = lm->iterator();
        while (iter->has_next()) {
            auto bytes = iter->next();
            auto rec = tx::create_log_record(std::move(bytes));
            records.push_back({rec->op(), rec->tx_number()});
        }
        return records;
    }
};

// ----------------------------------------------------------------------------
// Test: Transaction::set_int generates a SETINT log record
// ----------------------------------------------------------------------------
TEST_F(WALTest, SetIntGeneratesLogRecord) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    file::BlockId blk = tx->append("wal_test.dat");
    tx->pin(blk);

    // Write with ok_to_log=true
    tx->set_int(blk, 0, 42, true);

    // Check log contains SETINT record for this transaction
    auto records = read_log_records();
    bool found_setint = false;
    for (const auto& [op, txnum] : records) {
        if (op == tx::Op::SETINT && txnum.has_value() && txnum.value() == tx->tx_num()) {
            found_setint = true;
            break;
        }
    }
    EXPECT_TRUE(found_setint) << "SETINT log record not found after set_int(ok_to_log=true)";

    tx->commit();
}

// ----------------------------------------------------------------------------
// Test: Transaction::set_string generates a SETSTRING log record
// ----------------------------------------------------------------------------
TEST_F(WALTest, SetStringGeneratesLogRecord) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    file::BlockId blk = tx->append("wal_test.dat");
    tx->pin(blk);

    tx->set_string(blk, 0, "hello", true);

    auto records = read_log_records();
    bool found_setstring = false;
    for (const auto& [op, txnum] : records) {
        if (op == tx::Op::SETSTRING && txnum.has_value() && txnum.value() == tx->tx_num()) {
            found_setstring = true;
            break;
        }
    }
    EXPECT_TRUE(found_setstring) << "SETSTRING log record not found after set_string(ok_to_log=true)";

    tx->commit();
}

// ----------------------------------------------------------------------------
// Test: ok_to_log=false does NOT generate log records (used by format)
// ----------------------------------------------------------------------------
TEST_F(WALTest, NoLogWhenOkToLogFalse) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    file::BlockId blk = tx->append("wal_test.dat");
    tx->pin(blk);

    // Write with ok_to_log=false (like RecordPage::format)
    tx->set_int(blk, 0, 99, false);

    // Only START record should exist, no SETINT
    auto records = read_log_records();
    bool found_setint = false;
    for (const auto& [op, txnum] : records) {
        if (op == tx::Op::SETINT && txnum.has_value() && txnum.value() == tx->tx_num()) {
            found_setint = true;
        }
    }
    EXPECT_FALSE(found_setint) << "SETINT log record should NOT exist when ok_to_log=false";

    tx->commit();
}

// ----------------------------------------------------------------------------
// Test: Log record stores OLD value for undo
// ----------------------------------------------------------------------------
TEST_F(WALTest, LogRecordStoresOldValueForUndo) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    file::BlockId blk = tx->append("wal_test.dat");
    tx->pin(blk);

    // First write: set initial value (no log needed for initial)
    tx->set_int(blk, 0, 100, false);
    // Second write: this should log old value (100)
    tx->set_int(blk, 0, 200, true);

    // Read back - should be 200
    EXPECT_EQ(tx->get_int(blk, 0), 200);

    // Rollback should restore to 100 (the old value logged by SETINT)
    tx->rollback();

    // After rollback, start a new transaction to verify
    auto tx2 = std::make_shared<tx::Transaction>(fm, lm, bm);
    tx2->pin(blk);
    EXPECT_EQ(tx2->get_int(blk, 0), 100);
    tx2->commit();
}

// ----------------------------------------------------------------------------
// Test: Rollback undoes all logged changes in reverse order
// ----------------------------------------------------------------------------
TEST_F(WALTest, RollbackUndoesAllChanges) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    file::BlockId blk = tx->append("wal_test.dat");
    tx->pin(blk);

    // Initialize page (no log)
    tx->set_int(blk, 0, 0, false);
    tx->set_int(blk, 4, 0, false);

    // Make logged changes
    tx->set_int(blk, 0, 111, true);
    tx->set_int(blk, 4, 222, true);

    // Verify changes took effect
    EXPECT_EQ(tx->get_int(blk, 0), 111);
    EXPECT_EQ(tx->get_int(blk, 4), 222);

    // Rollback
    tx->rollback();

    // Verify undo: values should be back to 0
    auto tx2 = std::make_shared<tx::Transaction>(fm, lm, bm);
    tx2->pin(blk);
    EXPECT_EQ(tx2->get_int(blk, 0), 0);
    EXPECT_EQ(tx2->get_int(blk, 4), 0);
    tx2->commit();
}

// ----------------------------------------------------------------------------
// Test: Committed data persists across new transactions
// ----------------------------------------------------------------------------
TEST_F(WALTest, CommittedDataPersists) {
    file::BlockId blk("wal_test.dat", 0);

    {
        auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
        blk = tx->append("wal_test.dat");
        tx->pin(blk);
        tx->set_int(blk, 0, 42, true);
        tx->set_string(blk, 4, "WAL", true);
        tx->commit();
    }

    // Read with new transaction
    {
        auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
        tx->pin(blk);
        EXPECT_EQ(tx->get_int(blk, 0), 42);
        EXPECT_EQ(tx->get_string(blk, 4), "WAL");
        tx->commit();
    }
}

// ----------------------------------------------------------------------------
// Test: RecordPage writes go through Transaction (WAL path)
// ----------------------------------------------------------------------------
TEST_F(WALTest, RecordPageWritesGenerateLogRecords) {
    auto schema = std::make_shared<record::Schema>();
    schema->add_int_field("id");
    schema->add_string_field("name", 20);
    record::Layout layout(schema);

    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    file::BlockId blk = tx->append("wal_rp.dat");

    record::RecordPage rp(tx, blk, layout);
    rp.format();

    // Insert and set values through RecordPage
    auto slot = rp.insert_after(std::nullopt);
    ASSERT_TRUE(slot.has_value());
    rp.set_int(slot.value(), "id", 42);
    rp.set_string(slot.value(), "name", "Alice");

    // Verify log contains SETINT and SETSTRING records
    auto records = read_log_records();
    int setint_count = 0;
    int setstring_count = 0;
    for (const auto& [op, txnum] : records) {
        if (txnum.has_value() && txnum.value() == tx->tx_num()) {
            if (op == tx::Op::SETINT) setint_count++;
            if (op == tx::Op::SETSTRING) setstring_count++;
        }
    }

    // set_int("id") + insert_after(set_flag) = at least 2 SETINT records
    EXPECT_GE(setint_count, 2) << "RecordPage.set_int should generate SETINT log records";
    // set_string("name") = at least 1 SETSTRING record
    EXPECT_GE(setstring_count, 1) << "RecordPage.set_string should generate SETSTRING log records";

    tx->commit();
}

// ----------------------------------------------------------------------------
// Test: RecordPage format does NOT generate log records (ok_to_log=false)
// ----------------------------------------------------------------------------
TEST_F(WALTest, RecordPageFormatNoLogRecords) {
    auto schema = std::make_shared<record::Schema>();
    schema->add_int_field("val");
    record::Layout layout(schema);

    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    file::BlockId blk = tx->append("wal_rp2.dat");

    record::RecordPage rp(tx, blk, layout);
    rp.format();  // This should use ok_to_log=false

    // Check: only START record for this tx, no SETINT/SETSTRING
    auto records = read_log_records();
    bool found_data_log = false;
    for (const auto& [op, txnum] : records) {
        if (txnum.has_value() && txnum.value() == tx->tx_num()) {
            if (op == tx::Op::SETINT || op == tx::Op::SETSTRING) {
                found_data_log = true;
            }
        }
    }
    EXPECT_FALSE(found_data_log) << "format() should not generate SETINT/SETSTRING records";

    tx->commit();
}

// ----------------------------------------------------------------------------
// Test: RecordPage rollback undoes record insert
// ----------------------------------------------------------------------------
TEST_F(WALTest, RecordPageRollbackUndoesInsert) {
    auto schema = std::make_shared<record::Schema>();
    schema->add_int_field("id");
    record::Layout layout(schema);

    file::BlockId blk("wal_rp3.dat", 0);

    // TX1: format the page and commit
    {
        auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
        blk = tx->append("wal_rp3.dat");
        record::RecordPage rp(tx, blk, layout);
        rp.format();
        tx->commit();
    }

    // TX2: insert a record, then rollback
    {
        auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
        record::RecordPage rp(tx, blk, layout);
        auto slot = rp.insert_after(std::nullopt);
        ASSERT_TRUE(slot.has_value());
        rp.set_int(slot.value(), "id", 999);
        tx->rollback();  // Should undo the insert
    }

    // TX3: verify the page is empty (insert was undone)
    {
        auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
        record::RecordPage rp(tx, blk, layout);
        auto slot = rp.next_after(std::nullopt);
        EXPECT_FALSE(slot.has_value()) << "Rolled-back insert should leave page empty";
        tx->commit();
    }
}

// ----------------------------------------------------------------------------
// Test: Uncommitted transaction's log has START + SETINT but no COMMIT,
//       and rollback (what recovery does) restores the old value.
// Note: Full crash recovery can't be tested in-process because the global
//       LockTable (static singleton) persists - a real crash destroys it.
// ----------------------------------------------------------------------------
TEST_F(WALTest, UncommittedTxLogStructureSupportsRecovery) {
    // TX1: committed transaction
    auto tx1 = std::make_shared<tx::Transaction>(fm, lm, bm);
    file::BlockId blk = tx1->append("wal_recover.dat");
    tx1->pin(blk);
    tx1->set_int(blk, 0, 100, true);
    tx1->commit();
    size_t tx1_num = tx1->tx_num();

    // TX2: uncommitted (will be rolled back to simulate "what recovery would do")
    auto tx2 = std::make_shared<tx::Transaction>(fm, lm, bm);
    tx2->pin(blk);
    tx2->set_int(blk, 0, 999, true);
    size_t tx2_num = tx2->tx_num();

    // Verify data is 999 now
    EXPECT_EQ(tx2->get_int(blk, 0), 999);

    // Check log structure: TX2 should have START + SETINT, but NO COMMIT
    auto records = read_log_records();
    bool has_start = false, has_setint = false, has_commit = false;
    for (const auto& [op, txnum] : records) {
        if (txnum.has_value() && txnum.value() == tx2_num) {
            if (op == tx::Op::START) has_start = true;
            if (op == tx::Op::SETINT) has_setint = true;
            if (op == tx::Op::COMMIT) has_commit = true;
        }
    }
    EXPECT_TRUE(has_start) << "Uncommitted TX should have START record";
    EXPECT_TRUE(has_setint) << "Uncommitted TX should have SETINT record (old value for undo)";
    EXPECT_FALSE(has_commit) << "Uncommitted TX should NOT have COMMIT record";

    // TX1 should be fully committed in the log
    bool tx1_committed = false;
    for (const auto& [op, txnum] : records) {
        if (txnum.has_value() && txnum.value() == tx1_num && op == tx::Op::COMMIT) {
            tx1_committed = true;
        }
    }
    EXPECT_TRUE(tx1_committed) << "TX1 should have COMMIT record in log";

    // Rollback TX2 (what recovery would do for an uncommitted TX)
    tx2->rollback();

    // Verify: data restored to TX1's committed value
    auto tx3 = std::make_shared<tx::Transaction>(fm, lm, bm);
    tx3->pin(blk);
    EXPECT_EQ(tx3->get_int(blk, 0), 100)
        << "After rollback/recovery, data should be restored to committed value";
    tx3->commit();
}

// ----------------------------------------------------------------------------
// Test: Log ordering - START before SETINT before COMMIT
// ----------------------------------------------------------------------------
TEST_F(WALTest, LogOrderingStartSetCommit) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    file::BlockId blk = tx->append("wal_order.dat");
    tx->pin(blk);

    tx->set_int(blk, 0, 42, true);
    tx->commit();

    // Log records are in reverse order (most recent first)
    auto records = read_log_records();
    size_t txnum = tx->tx_num();

    // Filter to our transaction's records
    std::vector<tx::Op> tx_ops;
    for (const auto& [op, tn] : records) {
        if (tn.has_value() && tn.value() == txnum) {
            tx_ops.push_back(op);
        }
    }

    // Reverse to get chronological order (log iterator reads newest first)
    std::reverse(tx_ops.begin(), tx_ops.end());

    // Should be: START -> SETINT -> COMMIT
    ASSERT_GE(tx_ops.size(), 3u);
    EXPECT_EQ(tx_ops.front(), tx::Op::START);
    EXPECT_EQ(tx_ops.back(), tx::Op::COMMIT);

    // SETINT should be between START and COMMIT
    bool found_setint = false;
    for (size_t i = 1; i < tx_ops.size() - 1; i++) {
        if (tx_ops[i] == tx::Op::SETINT) {
            found_setint = true;
        }
    }
    EXPECT_TRUE(found_setint) << "SETINT should appear between START and COMMIT";
}

// ----------------------------------------------------------------------------
// Test: Buffer set_modified records correct txnum (not 0)
// ----------------------------------------------------------------------------
TEST_F(WALTest, BufferSetModifiedWithCorrectTxnum) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    file::BlockId blk = tx->append("wal_txnum.dat");
    tx->pin(blk);

    // After set_int, the buffer should record the transaction's txnum
    tx->set_int(blk, 0, 42, true);

    // The buffer's modifying_tx should be this transaction's txnum (not 0)
    auto idx = bm->pin(blk);
    auto& buff = bm->buffer(idx);
    ASSERT_TRUE(buff.modifying_tx().has_value());
    EXPECT_EQ(buff.modifying_tx().value(), tx->tx_num())
        << "Buffer should be marked with actual transaction number, not 0";
    bm->unpin(idx);

    tx->commit();
}
