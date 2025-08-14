#include "record/tablescan.hpp"
#include "record/schema.hpp"
#include "record/layout.hpp"
#include "tx/transaction.hpp"
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
    std::cout << "\n Phase 4 Demo - Record Management Layer (with Transaction)\n";

    // Setup database
    std::string db_dir = "/tmp/mudopdb_demo";

    if (fs::exists(db_dir)) {
        fs::remove_all(db_dir);
    }
    fs::create_directories(db_dir);

    auto fm = std::make_shared<FileMgr>(db_dir, 400);
    auto lm = std::make_shared<LogMgr>(fm, "demo.log");
    auto bm = std::make_shared<BufferMgr>(fm, lm, 8);

    // Create schema
    auto schema = std::make_shared<Schema>();
    schema->add_int_field("id");
    schema->add_string_field("name", 20);
    schema->add_int_field("age");
    schema->add_string_field("major", 15);

    Layout layout(schema);

    // Use Transaction
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    TableScan scan(tx, "students", layout);

    // Insert records
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
    };

    for (const auto& student : students) {
        scan.insert();
        scan.set_int("id", student.id);
        scan.set_string("name", student.name);
        scan.set_int("age", student.age);
        scan.set_string("major", student.major);
    }

    // Display all records
    print_header(*schema);
    scan.before_first();
    int count = 0;
    while (scan.next()) {
        print_record(scan, *schema);
        count++;
    }
    std::cout << "Total records: " << count << "\n";

    scan.close();
    tx->commit();

    // Cleanup
    fs::remove_all(db_dir);

    return 0;
}
