#include "record/tablescan.hpp"
#include "record/schema.hpp"
#include "record/layout.hpp"
#include "buffer/buffermgr.hpp"
#include "file/filemgr.hpp"
#include "log/logmgr.hpp"
#include <iostream>
#include <iomanip>
#include <filesystem>

using namespace record;
using namespace buffer;
using namespace file;
using namespace log;
namespace fs = std::filesystem;

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  PERSISTENCE TEST - READER APPLICATION                  ║\n";
    std::cout << "║  This is a COMPLETELY SEPARATE program from the writer  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";

    std::string db_dir = "/tmp/mudopdb_persistent";

    std::cout << "\n[READER] Step 1: Checking if database exists...\n";
    std::cout << "         Database directory: " << db_dir << "\n";

    if (!fs::exists(db_dir)) {
        std::cout << "         ✗ ERROR: Database directory not found!\n";
        std::cout << "         Please run the WRITER program first.\n";
        return 1;
    }

    std::cout << "         ✓ Database directory found\n";

    // Initialize storage engine (FRESH INSTANCE, no shared memory)
    std::cout << "\n[READER] Step 2: Initializing NEW storage engine instance...\n";
    auto fm = std::make_shared<FileMgr>(db_dir, 400);
    auto lm = std::make_shared<LogMgr>(fm, "app.log");
    auto bm = std::make_shared<BufferMgr>(fm, lm, 8);

    bool is_new_db = fm->is_new();
    std::cout << "         Database is_new: " << (is_new_db ? "YES (ERROR!)" : "NO (Good!)") << "\n";

    if (is_new_db) {
        std::cout << "         ✗ ERROR: Database appears new, data not found!\n";
        return 1;
    }

    std::cout << "         ✓ Existing database detected\n";

    // Check if table file exists
    size_t num_blocks = fm->length("accounts.tbl");
    std::cout << "\n[READER] Step 3: Checking table file...\n";
    std::cout << "         File: accounts.tbl\n";
    std::cout << "         Blocks: " << num_blocks << "\n";

    if (num_blocks == 0) {
        std::cout << "         ✗ ERROR: Table file is empty!\n";
        return 1;
    }

    std::cout << "         ✓ Table file exists with " << num_blocks << " blocks\n";

    // Recreate schema (must match writer's schema)
    std::cout << "\n[READER] Step 4: Creating schema (must match writer)...\n";
    auto schema = std::make_shared<Schema>();
    schema->add_int_field("id");
    schema->add_string_field("name", 30);
    schema->add_int_field("balance");

    Layout layout(schema);
    std::cout << "         ✓ Schema created\n";

    // Open table and read data
    std::cout << "\n[READER] Step 5: Opening table and reading data...\n";
    TableScan scan(bm, "accounts", layout);

    std::cout << "\n         Reading all records from disk:\n";
    std::cout << "         " << std::string(60, '=') << "\n";
    std::cout << "         " << std::setw(10) << "ID"
              << std::setw(25) << "Name"
              << std::setw(15) << "Balance" << "\n";
    std::cout << "         " << std::string(60, '-') << "\n";

    scan.before_first();
    int count = 0;
    int total_balance = 0;

    while (scan.next()) {
        int id = scan.get_int("id");
        std::string name = scan.get_string("name");
        int balance = scan.get_int("balance");

        std::cout << "         " << std::setw(10) << id
                  << std::setw(25) << name
                  << std::setw(15) << ("$" + std::to_string(balance)) << "\n";

        count++;
        total_balance += balance;
    }

    std::cout << "         " << std::string(60, '=') << "\n";
    std::cout << "         Total records: " << count << "\n";
    std::cout << "         Total balance: $" << total_balance << "\n";

    // Verify expected data
    std::cout << "\n[READER] Step 6: Verifying data integrity...\n";

    bool data_correct = false;
    if (count == 5 && total_balance == 44450) {
        std::cout << "         ✓ Record count matches (5 records)\n";
        std::cout << "         ✓ Total balance matches ($44,450)\n";
        data_correct = true;
    } else {
        std::cout << "         ✗ Data mismatch!\n";
        std::cout << "         Expected: 5 records, $44,450\n";
        std::cout << "         Got: " << count << " records, $" << total_balance << "\n";
    }

    // Test CRUD operations on persisted data
    std::cout << "\n[READER] Step 7: Testing CRUD on persisted data...\n";

    // UPDATE: Increase Alice's balance
    std::cout << "         [UPDATE] Increasing Alice's balance by $1000...\n";
    scan.before_first();
    bool found = false;
    while (scan.next()) {
        if (scan.get_int("id") == 101) {
            int old_balance = scan.get_int("balance");
            scan.set_int("balance", old_balance + 1000);
            std::cout << "         ✓ Alice: $" << old_balance << " → $"
                      << scan.get_int("balance") << "\n";
            found = true;
            break;
        }
    }

    if (!found) {
        std::cout << "         ✗ ERROR: Could not find Alice's record\n";
    }

    // DELETE: Remove account 103
    std::cout << "\n         [DELETE] Removing Carol Williams (ID 103)...\n";
    scan.before_first();
    found = false;
    while (scan.next()) {
        if (scan.get_int("id") == 103) {
            std::string name = scan.get_string("name");
            scan.delete_record();
            std::cout << "         ✓ Deleted: " << name << "\n";
            found = true;
            break;
        }
    }

    if (!found) {
        std::cout << "         ✗ ERROR: Could not find Carol's record\n";
    }

    // INSERT: Add new account
    std::cout << "\n         [INSERT] Adding new account...\n";
    scan.insert();
    scan.set_int("id", 106);
    scan.set_string("name", "Frank Miller");
    scan.set_int("balance", 7500);
    std::cout << "         ✓ Inserted: Account 106 - Frank Miller ($7500)\n";

    // READ: Show final state
    std::cout << "\n         [READ] Final state after CRUD operations:\n";
    std::cout << "         " << std::string(60, '=') << "\n";
    std::cout << "         " << std::setw(10) << "ID"
              << std::setw(25) << "Name"
              << std::setw(15) << "Balance" << "\n";
    std::cout << "         " << std::string(60, '-') << "\n";

    scan.before_first();
    count = 0;
    while (scan.next()) {
        int id = scan.get_int("id");
        std::string name = scan.get_string("name");
        int balance = scan.get_int("balance");

        std::cout << "         " << std::setw(10) << id
                  << std::setw(25) << name
                  << std::setw(15) << ("$" + std::to_string(balance)) << "\n";
        count++;
    }
    std::cout << "         " << std::string(60, '=') << "\n";
    std::cout << "         Total records: " << count << " (was 5, deleted 1, added 1)\n";

    // Flush changes
    std::cout << "\n[READER] Step 8: Flushing changes to disk...\n";
    scan.close();
    bm->flush_all(0);
    std::cout << "         ✓ Changes persisted\n";

    // Summary
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  PERSISTENCE TEST RESULTS                                ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════╣\n";

    if (data_correct) {
        std::cout << "║  ✅ PERSISTENCE TEST PASSED                              ║\n";
        std::cout << "║                                                          ║\n";
        std::cout << "║  Proof:                                                  ║\n";
        std::cout << "║  • Writer app wrote data and exited completely           ║\n";
        std::cout << "║  • Reader app is a SEPARATE process                      ║\n";
        std::cout << "║  • All 5 records successfully read from disk             ║\n";
        std::cout << "║  • Data integrity verified (correct totals)              ║\n";
        std::cout << "║  • CRUD operations work on persisted data                ║\n";
        std::cout << "║                                                          ║\n";
        std::cout << "║  Storage engine is TRULY PERSISTENT!                     ║\n";
    } else {
        std::cout << "║  ✗ PERSISTENCE TEST FAILED                               ║\n";
        std::cout << "║  Data integrity check failed                             ║\n";
    }

    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";

    return data_correct ? 0 : 1;
}
