#include "record/tablescan.hpp"
#include "record/schema.hpp"
#include "record/layout.hpp"
#include "buffer/buffermgr.hpp"
#include "file/filemgr.hpp"
#include "log/logmgr.hpp"
#include <iostream>
#include <filesystem>

using namespace record;
using namespace buffer;
using namespace file;
using namespace log;
namespace fs = std::filesystem;

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  PERSISTENCE TEST - WRITER APPLICATION                  ║\n";
    std::cout << "║  This program writes data and exits completely          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";

    std::string db_dir = "/tmp/mudopdb_persistent";

    std::cout << "\n[WRITER] Step 1: Setting up database...\n";
    std::cout << "         Database directory: " << db_dir << "\n";

    // Create database directory if it doesn't exist
    if (!fs::exists(db_dir)) {
        fs::create_directories(db_dir);
        std::cout << "         ✓ Created new database directory\n";
    } else {
        std::cout << "         ✓ Database directory already exists\n";
    }

    // Initialize storage engine
    auto fm = std::make_shared<FileMgr>(db_dir, 400);
    auto lm = std::make_shared<LogMgr>(fm, "app.log");
    auto bm = std::make_shared<BufferMgr>(fm, lm, 8);

    std::cout << "         ✓ Storage engine initialized\n";

    // Create schema
    std::cout << "\n[WRITER] Step 2: Creating table schema...\n";
    auto schema = std::make_shared<Schema>();
    schema->add_int_field("id");
    schema->add_string_field("name", 30);
    schema->add_int_field("balance");

    Layout layout(schema);
    std::cout << "         Table: ACCOUNTS\n";
    std::cout << "         Fields: id (INT), name (VARCHAR(30)), balance (INT)\n";

    // Create table and insert data
    TableScan scan(bm, "accounts", layout);

    std::cout << "\n[WRITER] Step 3: Inserting records...\n";

    struct Account {
        int id;
        std::string name;
        int balance;
    };

    Account accounts[] = {
        {101, "Alice Smith", 5000},
        {102, "Bob Johnson", 12500},
        {103, "Carol Williams", 8750},
        {104, "David Brown", 3200},
        {105, "Eve Davis", 15000}
    };

    for (const auto& acc : accounts) {
        scan.insert();
        scan.set_int("id", acc.id);
        scan.set_string("name", acc.name);
        scan.set_int("balance", acc.balance);
        std::cout << "         ✓ Inserted: Account " << acc.id << " - "
                  << acc.name << " ($" << acc.balance << ")\n";
    }

    // Verify what we wrote
    std::cout << "\n[WRITER] Step 4: Verifying data before exit...\n";
    scan.before_first();
    int count = 0;
    while (scan.next()) {
        count++;
    }
    std::cout << "         ✓ Verified " << count << " records in table\n";

    // Show storage stats
    size_t num_blocks = fm->length("accounts.tbl");
    std::cout << "\n[WRITER] Step 5: Storage statistics:\n";
    std::cout << "         File: accounts.tbl\n";
    std::cout << "         Blocks: " << num_blocks << "\n";
    std::cout << "         Size: " << (num_blocks * 400) << " bytes\n";

    // Explicitly close and flush
    std::cout << "\n[WRITER] Step 6: Closing database...\n";
    std::cout << "         Closing table scan...\n";
    scan.close();

    std::cout << "         Flushing all dirty buffers to disk...\n";
    bm->flush_all(0);

    std::cout << "         ✓ All data flushed to disk\n";

    // Now the destructors will run
    std::cout << "\n[WRITER] Step 7: Exiting application...\n";
    std::cout << "         Destructors will run, files will be closed\n";
    std::cout << "         Data should persist on disk at: " << db_dir << "\n";

    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  WRITER APPLICATION COMPLETE                             ║\n";
    std::cout << "║  Data has been written to disk and application is        ║\n";
    std::cout << "║  now exiting. Run the READER to verify persistence.      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";

    return 0;
}
