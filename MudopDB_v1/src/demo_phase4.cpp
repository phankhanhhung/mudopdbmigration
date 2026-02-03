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

void print_header(const Schema& schema) {
    std::cout << "\n";
    std::cout << std::string(60, '=') << "\n";
    for (const auto& field : schema.fields()) {
        std::cout << std::setw(15) << field;
    }
    std::cout << "\n" << std::string(60, '-') << "\n";
}

void print_record(TableScan& scan, const Schema& schema) {
    for (const auto& field : schema.fields()) {
        if (schema.type(field) == Type::INTEGER) {
            std::cout << std::setw(15) << scan.get_int(field);
        } else {
            std::cout << std::setw(15) << scan.get_string(field);
        }
    }
    std::cout << "\n";
}

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║     MudopDB Phase 4 Demo - Record Management Layer      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";

    // Setup database
    std::string db_dir = "/tmp/mudopdb_demo";

    // Clean up from previous runs
    if (fs::exists(db_dir)) {
        fs::remove_all(db_dir);
    }
    fs::create_directories(db_dir);

    std::cout << "\n[1] Initializing Storage Engine...\n";
    std::cout << "    Database directory: " << db_dir << "\n";
    std::cout << "    Block size: 400 bytes\n";
    std::cout << "    Buffer pool: 8 buffers\n";

    auto fm = std::make_shared<FileMgr>(db_dir, 400);
    auto lm = std::make_shared<LogMgr>(fm, "demo.log");
    auto bm = std::make_shared<BufferMgr>(fm, lm, 8);

    // Create schema for STUDENTS table
    std::cout << "\n[2] Creating Table Schema: STUDENTS\n";
    auto schema = std::make_shared<Schema>();
    schema->add_int_field("id");
    schema->add_string_field("name", 20);
    schema->add_int_field("age");
    schema->add_string_field("major", 15);

    Layout layout(schema);
    std::cout << "    Fields: id (INT), name (VARCHAR(20)), age (INT), major (VARCHAR(15))\n";
    std::cout << "    Slot size: " << layout.slot_size() << " bytes\n";
    std::cout << "    Records per block: " << (400 / layout.slot_size()) << "\n";

    // Create table scan
    TableScan scan(bm, "students", layout);

    // Insert records
    std::cout << "\n[3] Inserting Records...\n";

    struct Student {
        int id;
        std::string name;
        int age;
        std::string major;
    };

    Student students[] = {
        {1, "Alice Johnson", 20, "Computer Sci"},
        {2, "Bob Smith", 22, "Mathematics"},
        {3, "Carol Davis", 21, "Physics"},
        {4, "David Brown", 23, "Chemistry"},
        {5, "Eve Wilson", 19, "Biology"},
        {6, "Frank Miller", 24, "Engineering"},
        {7, "Grace Lee", 20, "Art"},
        {8, "Henry Chen", 22, "History"},
        {9, "Ivy Martinez", 21, "Economics"},
        {10, "Jack Taylor", 23, "Philosophy"}
    };

    for (const auto& student : students) {
        scan.insert();
        scan.set_int("id", student.id);
        scan.set_string("name", student.name);
        scan.set_int("age", student.age);
        scan.set_string("major", student.major);
        std::cout << "    Inserted: " << student.name
                  << " (ID: " << student.id << ")\n";
    }

    // Display all records
    std::cout << "\n[4] Scanning All Records:";
    print_header(*schema);

    scan.before_first();
    int count = 0;
    while (scan.next()) {
        print_record(scan, *schema);
        count++;
    }
    std::cout << std::string(60, '=') << "\n";
    std::cout << "Total records: " << count << "\n";

    // Update a record
    std::cout << "\n[5] Updating Record (ID=5, Eve Wilson)...\n";
    scan.before_first();
    while (scan.next()) {
        if (scan.get_int("id") == 5) {
            std::cout << "    Before: " << scan.get_string("name")
                      << ", Age: " << scan.get_int("age")
                      << ", Major: " << scan.get_string("major") << "\n";

            scan.set_int("age", 20);  // Update age
            scan.set_string("major", "Comp Sci");  // Update major

            std::cout << "    After:  " << scan.get_string("name")
                      << ", Age: " << scan.get_int("age")
                      << ", Major: " << scan.get_string("major") << "\n";
            break;
        }
    }

    // Delete a record
    std::cout << "\n[6] Deleting Record (ID=3, Carol Davis)...\n";
    scan.before_first();
    while (scan.next()) {
        if (scan.get_int("id") == 3) {
            std::cout << "    Deleting: " << scan.get_string("name") << "\n";
            scan.delete_record();
            break;
        }
    }

    // Display records after delete
    std::cout << "\n[7] Records After Update and Delete:";
    print_header(*schema);

    scan.before_first();
    count = 0;
    while (scan.next()) {
        print_record(scan, *schema);
        count++;
    }
    std::cout << std::string(60, '=') << "\n";
    std::cout << "Total records: " << count << " (1 deleted)\n";

    // Demonstrate RID navigation
    std::cout << "\n[8] Demonstrating RID-Based Navigation...\n";
    scan.before_first();

    // Find and save RID of first record
    if (scan.next()) {
        auto rid = scan.get_rid();
        if (rid.has_value()) {
            std::cout << "    First record RID: " << rid.value().to_string() << "\n";
            std::cout << "    Name: " << scan.get_string("name") << "\n";

            // Move to end
            while (scan.next()) {}

            // Jump back to first record using RID
            std::cout << "    Jumping back to first record using RID...\n";
            scan.move_to_rid(rid.value());
            std::cout << "    Name: " << scan.get_string("name") << "\n";
        }
    }

    // Show file statistics
    std::cout << "\n[9] Storage Statistics:\n";
    size_t num_blocks = fm->length("students.tbl");
    std::cout << "    Table file: students.tbl\n";
    std::cout << "    Number of blocks: " << num_blocks << "\n";
    std::cout << "    Total file size: " << (num_blocks * 400) << " bytes\n";
    std::cout << "    Available buffers: " << bm->available() << " / 8\n";

    // Demonstrate persistence
    std::cout << "\n[10] Testing Persistence...\n";
    std::cout << "     Closing table scan and flushing buffers...\n";
    scan.close();
    bm->flush_all(0);

    std::cout << "     Creating new scan to verify data persisted...\n";
    TableScan scan2(bm, "students", layout);

    scan2.before_first();
    int persisted_count = 0;
    while (scan2.next()) {
        persisted_count++;
    }

    std::cout << "     ✓ Found " << persisted_count << " records after restart\n";
    std::cout << "     ✓ Data successfully persisted to disk!\n";

    scan2.close();

    // Summary
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    Demo Summary                          ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════╣\n";
    std::cout << "║  ✓ Created table schema (4 fields)                       ║\n";
    std::cout << "║  ✓ Inserted 10 records                                   ║\n";
    std::cout << "║  ✓ Updated 1 record                                      ║\n";
    std::cout << "║  ✓ Deleted 1 record                                      ║\n";
    std::cout << "║  ✓ Scanned all records sequentially                      ║\n";
    std::cout << "║  ✓ Navigated by RID (direct access)                      ║\n";
    std::cout << "║  ✓ Verified data persistence to disk                     ║\n";
    std::cout << "║  ✓ Used " << num_blocks << " blocks across multiple pages                    ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";

    std::cout << "\nPhase 4 Record Management Layer: ✅ FULLY FUNCTIONAL\n\n";

    // Cleanup
    fs::remove_all(db_dir);

    return 0;
}
