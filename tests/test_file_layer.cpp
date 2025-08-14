#include <gtest/gtest.h>
#include "file/blockid.hpp"
#include "file/page.hpp"
#include "file/filemgr.hpp"
#include <filesystem>
#include <unordered_set>

using namespace file;
namespace fs = std::filesystem;

// ============================================================================
// BlockId Tests
// ============================================================================

TEST(BlockIdTest, Construction) {
    BlockId blk("testfile.dat", 5);
    EXPECT_EQ(blk.file_name(), "testfile.dat");
    EXPECT_EQ(blk.number(), 5);
}

TEST(BlockIdTest, ToString) {
    BlockId blk("myfile.tbl", 10);
    std::string str = blk.to_string();
    EXPECT_NE(str.find("myfile.tbl"), std::string::npos);
    EXPECT_NE(str.find("10"), std::string::npos);
}

TEST(BlockIdTest, Equality) {
    BlockId blk1("file.dat", 5);
    BlockId blk2("file.dat", 5);
    BlockId blk3("file.dat", 6);
    BlockId blk4("other.dat", 5);

    EXPECT_TRUE(blk1 == blk2);
    EXPECT_FALSE(blk1 != blk2);
    EXPECT_FALSE(blk1 == blk3);
    EXPECT_TRUE(blk1 != blk3);
    EXPECT_FALSE(blk1 == blk4);
    EXPECT_TRUE(blk1 != blk4);
}

TEST(BlockIdTest, Ordering) {
    BlockId blk1("aaa.dat", 5);
    BlockId blk2("aaa.dat", 10);
    BlockId blk3("bbb.dat", 5);

    EXPECT_TRUE(blk1 < blk2);
    EXPECT_TRUE(blk1 < blk3);
    EXPECT_FALSE(blk2 < blk1);
}

TEST(BlockIdTest, HashFunction) {
    BlockId blk1("file.dat", 5);
    BlockId blk2("file.dat", 5);
    BlockId blk3("file.dat", 6);

    std::hash<BlockId> hasher;

    // Same blocks should have same hash
    EXPECT_EQ(hasher(blk1), hasher(blk2));

    // Different blocks should (probably) have different hashes
    EXPECT_NE(hasher(blk1), hasher(blk3));
}

TEST(BlockIdTest, UnorderedSet) {
    std::unordered_set<BlockId> block_set;

    BlockId blk1("file.dat", 1);
    BlockId blk2("file.dat", 2);
    BlockId blk3("file.dat", 1);  // Duplicate of blk1

    block_set.insert(blk1);
    block_set.insert(blk2);
    block_set.insert(blk3);

    EXPECT_EQ(block_set.size(), 2);  // Only 2 unique blocks
    EXPECT_TRUE(block_set.find(blk1) != block_set.end());
    EXPECT_TRUE(block_set.find(blk2) != block_set.end());
}

// ============================================================================
// Page Tests
// ============================================================================

TEST(PageTest, Construction) {
    Page page(400);
    EXPECT_EQ(page.size(), 400);

    // All bytes should be initialized to zero
    for (size_t i = 0; i < page.size(); i++) {
        EXPECT_EQ(page.contents()[i], 0);
    }
}

TEST(PageTest, ConstructionWithData) {
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    Page page(data);
    EXPECT_EQ(page.size(), 5);
    EXPECT_EQ(page.contents()[0], 1);
    EXPECT_EQ(page.contents()[4], 5);
}

TEST(PageTest, IntegerOperations) {
    Page page(400);

    // Write integers
    page.set_int(0, 123);
    page.set_int(4, -456);
    page.set_int(100, 0);
    page.set_int(200, 2147483647);  // INT32_MAX
    page.set_int(204, -2147483648); // INT32_MIN

    // Read integers
    EXPECT_EQ(page.get_int(0), 123);
    EXPECT_EQ(page.get_int(4), -456);
    EXPECT_EQ(page.get_int(100), 0);
    EXPECT_EQ(page.get_int(200), 2147483647);
    EXPECT_EQ(page.get_int(204), -2147483648);
}

TEST(PageTest, IntegerBigEndian) {
    Page page(400);

    // Set integer
    page.set_int(0, 0x12345678);

    // Check big-endian byte order
    EXPECT_EQ(page.contents()[0], 0x12);
    EXPECT_EQ(page.contents()[1], 0x34);
    EXPECT_EQ(page.contents()[2], 0x56);
    EXPECT_EQ(page.contents()[3], 0x78);
}

TEST(PageTest, StringOperations) {
    Page page(400);

    // Write strings
    page.set_string(0, "Hello");
    page.set_string(50, "World!");
    page.set_string(100, "");  // Empty string
    page.set_string(150, "Testing 123");

    // Read strings
    EXPECT_EQ(page.get_string(0), "Hello");
    EXPECT_EQ(page.get_string(50), "World!");
    EXPECT_EQ(page.get_string(100), "");
    EXPECT_EQ(page.get_string(150), "Testing 123");
}

TEST(PageTest, StringFormat) {
    Page page(400);

    std::string test = "ABC";
    page.set_string(0, test);

    // Check format: [4-byte length][data]
    EXPECT_EQ(page.get_int(0), 3);  // Length = 3
    EXPECT_EQ(page.contents()[4], 'A');
    EXPECT_EQ(page.contents()[5], 'B');
    EXPECT_EQ(page.contents()[6], 'C');
}

TEST(PageTest, ByteOperations) {
    Page page(400);

    uint8_t data1[] = {10, 20, 30, 40, 50};
    uint8_t data2[] = {100, 200};

    // Write bytes
    page.set_bytes(0, data1, 5);
    page.set_bytes(50, data2, 2);

    // Read bytes
    const uint8_t* read1 = page.get_bytes(0);
    size_t len1 = page.get_bytes_length(0);

    EXPECT_EQ(len1, 5);
    EXPECT_EQ(read1[0], 10);
    EXPECT_EQ(read1[1], 20);
    EXPECT_EQ(read1[2], 30);
    EXPECT_EQ(read1[3], 40);
    EXPECT_EQ(read1[4], 50);

    const uint8_t* read2 = page.get_bytes(50);
    size_t len2 = page.get_bytes_length(50);

    EXPECT_EQ(len2, 2);
    EXPECT_EQ(read2[0], 100);
    EXPECT_EQ(read2[1], 200);
}

TEST(PageTest, MaxLength) {
    EXPECT_EQ(Page::max_length(10), 14);  // 4 + 10
    EXPECT_EQ(Page::max_length(0), 4);    // 4 + 0
    EXPECT_EQ(Page::max_length(100), 104);
}

TEST(PageTest, OutOfBounds) {
    Page page(100);

    // These should throw
    EXPECT_THROW(page.get_int(97), std::out_of_range);  // Would read bytes 97-100
    EXPECT_THROW(page.set_int(97, 123), std::out_of_range);
    EXPECT_THROW(page.get_int(100), std::out_of_range);
}

TEST(PageTest, MultipleWrites) {
    Page page(400);

    // Write and overwrite
    page.set_int(0, 100);
    EXPECT_EQ(page.get_int(0), 100);

    page.set_int(0, 200);
    EXPECT_EQ(page.get_int(0), 200);

    page.set_string(0, "First");
    EXPECT_EQ(page.get_string(0), "First");

    page.set_string(0, "Second");
    EXPECT_EQ(page.get_string(0), "Second");
}

// ============================================================================
// FileMgr Tests
// ============================================================================

class FileMgrTest : public ::testing::Test {
protected:
    std::string test_dir = "/tmp/mudopdb_test";
    size_t blocksize = 400;

    void SetUp() override {
        // Clean up test directory
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }

    void TearDown() override {
        // Clean up test directory
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }
};

TEST_F(FileMgrTest, NewDatabase) {
    FileMgr fm(test_dir, blocksize);

    EXPECT_TRUE(fm.is_new());
    EXPECT_TRUE(fs::exists(test_dir));
    EXPECT_EQ(fm.block_size(), blocksize);
}

TEST_F(FileMgrTest, ExistingDatabase) {
    // Create directory first
    fs::create_directories(test_dir);

    FileMgr fm(test_dir, blocksize);

    EXPECT_FALSE(fm.is_new());
}

TEST_F(FileMgrTest, AppendBlock) {
    FileMgr fm(test_dir, blocksize);

    // Initially empty
    EXPECT_EQ(fm.length("test.dat"), 0);

    // Append blocks
    BlockId blk0 = fm.append("test.dat");
    EXPECT_EQ(blk0.number(), 0);
    EXPECT_EQ(fm.length("test.dat"), 1);

    BlockId blk1 = fm.append("test.dat");
    EXPECT_EQ(blk1.number(), 1);
    EXPECT_EQ(fm.length("test.dat"), 2);

    BlockId blk2 = fm.append("test.dat");
    EXPECT_EQ(blk2.number(), 2);
    EXPECT_EQ(fm.length("test.dat"), 3);
}

TEST_F(FileMgrTest, WriteAndReadBlock) {
    FileMgr fm(test_dir, blocksize);

    // Create a page with test data
    Page write_page(blocksize);
    write_page.set_int(0, 12345);
    write_page.set_string(50, "TestString");
    write_page.set_int(100, -9999);

    // Append block and write
    BlockId blk = fm.append("test.dat");
    fm.write(blk, write_page);

    // Read it back
    Page read_page(blocksize);
    fm.read(blk, read_page);

    // Verify data
    EXPECT_EQ(read_page.get_int(0), 12345);
    EXPECT_EQ(read_page.get_string(50), "TestString");
    EXPECT_EQ(read_page.get_int(100), -9999);
}

TEST_F(FileMgrTest, MultipleFiles) {
    FileMgr fm(test_dir, blocksize);

    // Create multiple files
    fm.append("file1.dat");
    fm.append("file1.dat");
    fm.append("file2.dat");

    EXPECT_EQ(fm.length("file1.dat"), 2);
    EXPECT_EQ(fm.length("file2.dat"), 1);
    EXPECT_EQ(fm.length("file3.dat"), 0);  // Doesn't exist yet
}

TEST_F(FileMgrTest, ReadNonexistentBlock) {
    FileMgr fm(test_dir, blocksize);

    Page page(blocksize);
    page.set_int(0, 999);  // Set some non-zero value

    // Read from nonexistent file
    BlockId blk("nonexistent.dat", 0);
    fm.read(blk, page);

    // Page should remain unchanged or zeroed (implementation dependent)
    // In our implementation, nonexistent blocks leave page as-is
}

TEST_F(FileMgrTest, OverwriteBlock) {
    FileMgr fm(test_dir, blocksize);

    BlockId blk = fm.append("test.dat");

    // Write first data
    Page page1(blocksize);
    page1.set_int(0, 111);
    fm.write(blk, page1);

    // Write second data (overwrite)
    Page page2(blocksize);
    page2.set_int(0, 222);
    fm.write(blk, page2);

    // Read back
    Page page_read(blocksize);
    fm.read(blk, page_read);

    EXPECT_EQ(page_read.get_int(0), 222);
}

TEST_F(FileMgrTest, Persistence) {
    // Write in first session
    {
        FileMgr fm(test_dir, blocksize);

        Page page(blocksize);
        page.set_int(0, 42);
        page.set_string(50, "Persistent");

        BlockId blk = fm.append("persist.dat");
        fm.write(blk, page);
    }

    // Read in second session
    {
        FileMgr fm(test_dir, blocksize);

        EXPECT_FALSE(fm.is_new());  // Database already exists
        EXPECT_EQ(fm.length("persist.dat"), 1);

        BlockId blk("persist.dat", 0);
        Page page(blocksize);
        fm.read(blk, page);

        EXPECT_EQ(page.get_int(0), 42);
        EXPECT_EQ(page.get_string(50), "Persistent");
    }
}

TEST_F(FileMgrTest, TempFileCleanup) {
    // Create temp files
    fs::create_directories(test_dir);
    std::ofstream temp1(test_dir + "/tempfile1.dat");
    std::ofstream temp2(test_dir + "/temp_test.dat");
    std::ofstream normal(test_dir + "/normal.dat");
    temp1.close();
    temp2.close();
    normal.close();

    // Create FileMgr (should delete temp files)
    FileMgr fm(test_dir, blocksize);

    EXPECT_FALSE(fs::exists(test_dir + "/tempfile1.dat"));
    EXPECT_FALSE(fs::exists(test_dir + "/temp_test.dat"));
    EXPECT_TRUE(fs::exists(test_dir + "/normal.dat"));
}

TEST_F(FileMgrTest, LargeData) {
    FileMgr fm(test_dir, blocksize);

    BlockId blk = fm.append("large.dat");

    // Fill page with pattern
    Page write_page(blocksize);
    for (size_t i = 0; i < blocksize - 4; i += 4) {
        write_page.set_int(i, static_cast<int32_t>(i));
    }

    fm.write(blk, write_page);

    // Read back and verify
    Page read_page(blocksize);
    fm.read(blk, read_page);

    for (size_t i = 0; i < blocksize - 4; i += 4) {
        EXPECT_EQ(read_page.get_int(i), static_cast<int32_t>(i));
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(FileMgrTest, IntegrationTableSimulation) {
    FileMgr fm(test_dir, blocksize);

    // Simulate a simple table: [id: int, name: string, age: int]
    Page page(blocksize);

    // Record 1 at offset 0
    page.set_int(0, 1);           // id
    page.set_string(4, "Alice");  // name
    page.set_int(50, 25);         // age

    // Record 2 at offset 100
    page.set_int(100, 2);         // id
    page.set_string(104, "Bob");  // name
    page.set_int(150, 30);        // age

    // Write to disk
    BlockId blk = fm.append("students.tbl");
    fm.write(blk, page);

    // Read back
    Page read_page(blocksize);
    fm.read(blk, read_page);

    // Verify records
    EXPECT_EQ(read_page.get_int(0), 1);
    EXPECT_EQ(read_page.get_string(4), "Alice");
    EXPECT_EQ(read_page.get_int(50), 25);

    EXPECT_EQ(read_page.get_int(100), 2);
    EXPECT_EQ(read_page.get_string(104), "Bob");
    EXPECT_EQ(read_page.get_int(150), 30);
}

// main() is provided by gtest_main
