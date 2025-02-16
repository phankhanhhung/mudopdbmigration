#include <gtest/gtest.h>
#include "buffer/buffer.hpp"
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

class BufferTest : public ::testing::Test {
protected:
    std::string test_dir = "/tmp/mudopdb_buffer_test";
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
// Buffer Basic Tests
// ============================================================================

TEST_F(BufferTest, InitialState) {
    Buffer buf(fm, lm);

    // New buffer is unpinned and unassigned
    EXPECT_FALSE(buf.is_pinned());
    EXPECT_FALSE(buf.block().has_value());
    EXPECT_FALSE(buf.modifying_tx().has_value());
}

TEST_F(BufferTest, Contents) {
    Buffer buf(fm, lm);

    // Can access and modify contents
    buf.contents().set_int(0, 42);
    EXPECT_EQ(buf.contents().get_int(0), 42);

    buf.contents().set_string(10, "hello");
    EXPECT_EQ(buf.contents().get_string(10), "hello");
}

TEST_F(BufferTest, SetModified) {
    Buffer buf(fm, lm);

    // Initially not modified
    EXPECT_FALSE(buf.modifying_tx().has_value());

    // Set modified with txnum only
    buf.set_modified(1, std::nullopt);
    EXPECT_TRUE(buf.modifying_tx().has_value());
    EXPECT_EQ(buf.modifying_tx().value(), 1);

    // Set modified with txnum and lsn
    buf.set_modified(2, 100);
    EXPECT_EQ(buf.modifying_tx().value(), 2);
}

TEST_F(BufferTest, PinUnpin) {
    Buffer buf(fm, lm);

    // Initially unpinned
    EXPECT_FALSE(buf.is_pinned());

    // Pin increments count
    buf.pin();
    EXPECT_TRUE(buf.is_pinned());

    // Unpin decrements count
    buf.unpin();
    EXPECT_FALSE(buf.is_pinned());
}

TEST_F(BufferTest, MultiplePins) {
    Buffer buf(fm, lm);

    // Multiple pins require multiple unpins
    buf.pin();
    buf.pin();
    buf.pin();
    EXPECT_TRUE(buf.is_pinned());

    buf.unpin();
    EXPECT_TRUE(buf.is_pinned());

    buf.unpin();
    EXPECT_TRUE(buf.is_pinned());

    buf.unpin();
    EXPECT_FALSE(buf.is_pinned());
}

TEST_F(BufferTest, AssignToBlock) {
    Buffer buf(fm, lm);

    // Create a block with data
    BlockId blk("testfile.dat", 0);
    fm->append("testfile.dat");

    Page write_page(blocksize);
    write_page.set_int(0, 999);
    fm->write(blk, write_page);

    // Assign buffer to block
    buf.assign_to_block(blk);

    // Buffer should be assigned and data loaded
    EXPECT_TRUE(buf.block().has_value());
    EXPECT_EQ(buf.block().value(), blk);
    EXPECT_EQ(buf.contents().get_int(0), 999);
    EXPECT_FALSE(buf.is_pinned());  // assign_to_block resets pins to 0
}

TEST_F(BufferTest, FlushUnmodifiedBufferIsNoop) {
    Buffer buf(fm, lm);

    // Create and assign a block
    BlockId blk("testfile.dat", 0);
    fm->append("testfile.dat");
    buf.assign_to_block(blk);

    // Flush unmodified buffer - should not throw
    buf.flush();

    // Buffer should still be clean
    EXPECT_FALSE(buf.modifying_tx().has_value());
}

TEST_F(BufferTest, FlushDirtyBuffer) {
    Buffer buf(fm, lm);

    // Create and assign a block
    BlockId blk("testfile.dat", 0);
    fm->append("testfile.dat");
    buf.assign_to_block(blk);

    // Modify buffer
    buf.contents().set_int(0, 123);
    buf.set_modified(1, std::nullopt);

    // Flush should write to disk
    buf.flush();

    // Read back in new buffer to verify
    Page read_page(blocksize);
    fm->read(blk, read_page);
    EXPECT_EQ(read_page.get_int(0), 123);
}

TEST_F(BufferTest, FlushClearsModification) {
    Buffer buf(fm, lm);

    // Create and assign a block
    BlockId blk("testfile.dat", 0);
    fm->append("testfile.dat");
    buf.assign_to_block(blk);

    // Modify and flush
    buf.contents().set_int(0, 456);
    buf.set_modified(1, std::nullopt);
    EXPECT_TRUE(buf.modifying_tx().has_value());

    buf.flush();

    // After flush, txnum should be cleared
    EXPECT_FALSE(buf.modifying_tx().has_value());
}

TEST_F(BufferTest, ReassignFlushesOldBlock) {
    Buffer buf(fm, lm);

    // Create first block
    BlockId blk1("file1.dat", 0);
    fm->append("file1.dat");
    buf.assign_to_block(blk1);

    // Modify it
    buf.contents().set_int(0, 111);
    buf.set_modified(1, std::nullopt);

    // Create second block
    BlockId blk2("file2.dat", 0);
    fm->append("file2.dat");

    // Reassign to second block - should flush first block
    buf.assign_to_block(blk2);

    // Verify first block was flushed
    Page read_page(blocksize);
    fm->read(blk1, read_page);
    EXPECT_EQ(read_page.get_int(0), 111);

    // Buffer should now be assigned to second block and clean
    EXPECT_EQ(buf.block().value(), blk2);
    EXPECT_FALSE(buf.modifying_tx().has_value());
}

TEST_F(BufferTest, FlushRespectsWAL) {
    Buffer buf(fm, lm);

    // Create and assign a block
    BlockId blk("testfile.dat", 0);
    fm->append("testfile.dat");
    buf.assign_to_block(blk);

    // Modify with LSN
    buf.contents().set_int(0, 789);
    size_t lsn = lm->append(std::vector<uint8_t>{1, 2, 3});
    buf.set_modified(1, lsn);

    // Flush should flush log first, then data
    // (We can't directly test ordering, but at least verify it doesn't throw)
    buf.flush();

    // Verify data was written
    Page read_page(blocksize);
    fm->read(blk, read_page);
    EXPECT_EQ(read_page.get_int(0), 789);
}

// main() is provided by gtest_main
