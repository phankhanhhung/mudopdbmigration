#include <gtest/gtest.h>
#include "buffer/buffermgr.hpp"
#include "file/filemgr.hpp"
#include "log/logmgr.hpp"
#include <filesystem>
#include <memory>

using namespace buffer;
using namespace file;
using namespace log;
namespace fs = std::filesystem;

// ============================================================================
// Test Fixture
// ============================================================================

class BufferMgrTest : public ::testing::Test {
protected:
    std::string test_dir = "/tmp/mudopdb_buffermgr_test";
    std::string logfile = "test.log";
    size_t blocksize = 400;

    std::shared_ptr<FileMgr> fm;
    std::shared_ptr<LogMgr> lm;

    void SetUp() override {
        // Clean up test directory
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
        fs::create_directories(test_dir);

        // Create managers
        fm = std::make_shared<FileMgr>(test_dir, blocksize);
        lm = std::make_shared<LogMgr>(fm, logfile);
    }

    void TearDown() override {
        // Clean up test directory
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }
};

// ============================================================================
// BufferMgr Basic Tests
// ============================================================================

TEST_F(BufferMgrTest, InitialState) {
    BufferMgr bm(fm, lm, 3);

    // All buffers available initially
    EXPECT_EQ(bm.available(), 3);
}

TEST_F(BufferMgrTest, PinReducesAvailable) {
    BufferMgr bm(fm, lm, 3);

    // Create a block
    BlockId blk("testfile.dat", 0);
    fm->append("testfile.dat");

    // Pin reduces available count
    EXPECT_EQ(bm.available(), 3);
    bm.pin(blk);
    EXPECT_EQ(bm.available(), 2);
}

TEST_F(BufferMgrTest, UnpinIncreasesAvailable) {
    BufferMgr bm(fm, lm, 3);

    // Create a block and pin it
    BlockId blk("testfile.dat", 0);
    fm->append("testfile.dat");
    size_t idx = bm.pin(blk);

    EXPECT_EQ(bm.available(), 2);

    // Unpin increases available count
    bm.unpin(idx);
    EXPECT_EQ(bm.available(), 3);
}

TEST_F(BufferMgrTest, PinSameBlockTwice) {
    BufferMgr bm(fm, lm, 3);

    // Create a block
    BlockId blk("testfile.dat", 0);
    fm->append("testfile.dat");

    // Pin same block twice returns same index
    size_t idx1 = bm.pin(blk);
    size_t idx2 = bm.pin(blk);

    EXPECT_EQ(idx1, idx2);
    // Available reduced by 1 (not 2) because same buffer
    EXPECT_EQ(bm.available(), 2);
}

TEST_F(BufferMgrTest, MultiplePinsSameBuffer) {
    BufferMgr bm(fm, lm, 3);

    // Create a block
    BlockId blk("testfile.dat", 0);
    fm->append("testfile.dat");

    // Pin multiple times
    size_t idx = bm.pin(blk);
    bm.pin(blk);
    bm.pin(blk);

    EXPECT_EQ(bm.available(), 2);

    // First unpin doesn't free buffer
    bm.unpin(idx);
    EXPECT_EQ(bm.available(), 2);

    // Second unpin still doesn't free
    bm.unpin(idx);
    EXPECT_EQ(bm.available(), 2);

    // Third unpin frees buffer
    bm.unpin(idx);
    EXPECT_EQ(bm.available(), 3);
}

TEST_F(BufferMgrTest, PinDifferentBlocks) {
    BufferMgr bm(fm, lm, 3);

    // Create multiple blocks
    BlockId blk1("file1.dat", 0);
    BlockId blk2("file2.dat", 0);
    BlockId blk3("file3.dat", 0);

    fm->append("file1.dat");
    fm->append("file2.dat");
    fm->append("file3.dat");

    // Pin different blocks uses different buffers
    size_t idx1 = bm.pin(blk1);
    size_t idx2 = bm.pin(blk2);
    size_t idx3 = bm.pin(blk3);

    EXPECT_NE(idx1, idx2);
    EXPECT_NE(idx2, idx3);
    EXPECT_NE(idx1, idx3);

    EXPECT_EQ(bm.available(), 0);
}

TEST_F(BufferMgrTest, Eviction) {
    BufferMgr bm(fm, lm, 2);

    // Create blocks
    BlockId blk1("file1.dat", 0);
    BlockId blk2("file2.dat", 0);
    BlockId blk3("file3.dat", 0);

    fm->append("file1.dat");
    fm->append("file2.dat");
    fm->append("file3.dat");

    // Pin and unpin first block
    size_t idx1 = bm.pin(blk1);
    bm.buffer(idx1).contents().set_int(0, 111);
    bm.unpin(idx1);

    // Pin second block
    size_t idx2 = bm.pin(blk2);
    bm.buffer(idx2).contents().set_int(0, 222);
    bm.unpin(idx2);

    // Pin third block - should evict one of the unpinned buffers
    size_t idx3 = bm.pin(blk3);
    EXPECT_TRUE(idx3 == idx1 || idx3 == idx2);  // Reused one of the buffers

    bm.unpin(idx3);
}

TEST_F(BufferMgrTest, FlushAll) {
    BufferMgr bm(fm, lm, 3);

    // Create blocks
    BlockId blk1("file1.dat", 0);
    BlockId blk2("file2.dat", 0);

    fm->append("file1.dat");
    fm->append("file2.dat");

    // Pin and modify with different transactions
    size_t idx1 = bm.pin(blk1);
    bm.buffer(idx1).contents().set_int(0, 111);
    bm.buffer(idx1).set_modified(1, std::nullopt);

    size_t idx2 = bm.pin(blk2);
    bm.buffer(idx2).contents().set_int(0, 222);
    bm.buffer(idx2).set_modified(2, std::nullopt);

    // flush_all(1) should only flush transaction 1
    bm.flush_all(1);

    // Buffer 1 should be clean, buffer 2 still dirty
    EXPECT_FALSE(bm.buffer(idx1).modifying_tx().has_value());
    EXPECT_TRUE(bm.buffer(idx2).modifying_tx().has_value());

    bm.unpin(idx1);
    bm.unpin(idx2);
}

TEST_F(BufferMgrTest, PinAfterUnpin) {
    BufferMgr bm(fm, lm, 2);

    // Create block
    BlockId blk("testfile.dat", 0);
    fm->append("testfile.dat");

    // Pin, modify, unpin
    size_t idx1 = bm.pin(blk);
    bm.buffer(idx1).contents().set_int(0, 999);
    bm.buffer(idx1).set_modified(1, std::nullopt);
    bm.unpin(idx1);

    // Pin again - should get same buffer with modifications
    size_t idx2 = bm.pin(blk);
    EXPECT_EQ(idx1, idx2);
    EXPECT_EQ(bm.buffer(idx2).contents().get_int(0), 999);

    bm.unpin(idx2);
}

TEST_F(BufferMgrTest, BufferContentsPersist) {
    BufferMgr bm(fm, lm, 2);

    // Create block
    BlockId blk("testfile.dat", 0);
    fm->append("testfile.dat");

    // Pin, write data, flush, unpin
    size_t idx1 = bm.pin(blk);
    bm.buffer(idx1).contents().set_int(0, 777);
    bm.buffer(idx1).set_modified(1, std::nullopt);
    bm.buffer(idx1).flush();
    bm.unpin(idx1);

    // Evict by filling pool
    BlockId blk2("file2.dat", 0);
    BlockId blk3("file3.dat", 0);
    fm->append("file2.dat");
    fm->append("file3.dat");

    size_t idx2 = bm.pin(blk2);
    size_t idx3 = bm.pin(blk3);

    // Unpin one to make room for repinning original block
    bm.unpin(idx2);

    // Pin original block again - may be in different buffer
    size_t idx4 = bm.pin(blk);
    EXPECT_EQ(bm.buffer(idx4).contents().get_int(0), 777);

    bm.unpin(idx3);
    bm.unpin(idx4);
}

TEST_F(BufferMgrTest, WALComplianceOnEviction) {
    BufferMgr bm(fm, lm, 2);

    // Create blocks
    BlockId blk1("file1.dat", 0);
    BlockId blk2("file2.dat", 0);
    BlockId blk3("file3.dat", 0);

    fm->append("file1.dat");
    fm->append("file2.dat");
    fm->append("file3.dat");

    // Pin and modify with LSN
    size_t idx1 = bm.pin(blk1);
    bm.buffer(idx1).contents().set_int(0, 123);
    size_t lsn = lm->append(std::vector<uint8_t>{1, 2, 3});
    bm.buffer(idx1).set_modified(1, lsn);
    bm.unpin(idx1);

    // Fill pool
    size_t idx2 = bm.pin(blk2);
    bm.unpin(idx2);

    // Pin third block - should evict blk1 and flush it (with WAL)
    size_t idx3 = bm.pin(blk3);
    bm.unpin(idx3);

    // Verify data was persisted
    Page read_page(blocksize);
    fm->read(blk1, read_page);
    EXPECT_EQ(read_page.get_int(0), 123);
}

TEST_F(BufferMgrTest, TimeoutThrows) {
    BufferMgr bm(fm, lm, 2);
    bm.set_max_time(100);  // 100ms timeout

    // Create blocks
    BlockId blk1("file1.dat", 0);
    BlockId blk2("file2.dat", 0);
    BlockId blk3("file3.dat", 0);

    fm->append("file1.dat");
    fm->append("file2.dat");
    fm->append("file3.dat");

    // Pin two blocks (exhaust pool)
    size_t idx1 = bm.pin(blk1);
    size_t idx2 = bm.pin(blk2);

    EXPECT_EQ(bm.available(), 0);

    // Try to pin third block - should timeout and throw
    EXPECT_THROW(bm.pin(blk3), BufferAbortException);

    bm.unpin(idx1);
    bm.unpin(idx2);
}

TEST_F(BufferMgrTest, LargePool) {
    BufferMgr bm(fm, lm, 100);

    EXPECT_EQ(bm.available(), 100);

    // Pin 50 buffers
    std::vector<size_t> indices;
    for (int i = 0; i < 50; i++) {
        std::string filename = "file" + std::to_string(i) + ".dat";
        BlockId blk(filename, 0);
        fm->append(filename);
        indices.push_back(bm.pin(blk));
    }

    EXPECT_EQ(bm.available(), 50);

    // Unpin all
    for (size_t idx : indices) {
        bm.unpin(idx);
    }

    EXPECT_EQ(bm.available(), 100);
}

TEST_F(BufferMgrTest, ExhaustPoolThenRecover) {
    BufferMgr bm(fm, lm, 2);
    bm.set_max_time(500);  // 500ms timeout

    // Create blocks
    BlockId blk1("file1.dat", 0);
    BlockId blk2("file2.dat", 0);
    BlockId blk3("file3.dat", 0);

    fm->append("file1.dat");
    fm->append("file2.dat");
    fm->append("file3.dat");

    // Exhaust pool
    size_t idx1 = bm.pin(blk1);
    size_t idx2 = bm.pin(blk2);

    // In separate thread, unpin after 200ms to allow third pin to succeed
    std::thread unpinner([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        bm.unpin(idx1);
    });

    // This should wait ~200ms then succeed
    size_t idx3 = bm.pin(blk3);
    EXPECT_TRUE(idx3 == idx1 || idx3 == idx2);

    unpinner.join();
    bm.unpin(idx2);
    bm.unpin(idx3);
}

// main() is provided by gtest_main
