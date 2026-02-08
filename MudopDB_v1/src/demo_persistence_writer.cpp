#include "record/tablescan.hpp"
#include "record/schema.hpp"
#include "record/layout.hpp"
#include "tx/transaction.hpp"
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
    std::string db_dir = "/tmp/mudopdb_persistent";

    if (!fs::exists(db_dir)) {
        fs::create_directories(db_dir);
    }

    auto fm = std::make_shared<FileMgr>(db_dir, 400);
    auto lm = std::make_shared<LogMgr>(fm, "app.log");
    auto bm = std::make_shared<BufferMgr>(fm, lm, 8);

    auto schema = std::make_shared<Schema>();
    schema->add_int_field("id");
    schema->add_string_field("name", 30);
    schema->add_int_field("balance");

    Layout layout(schema);

    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    TableScan scan(tx, "accounts", layout);

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
        scan.insert().value();
        scan.set_int("id", acc.id).value();
        scan.set_string("name", acc.name).value();
        scan.set_int("balance", acc.balance).value();
        std::cout << "Inserted: Account " << acc.id << " - " << acc.name << "\n";
    }

    scan.close().value();
    tx->commit();

    std::cout << "Writer done. Data persisted.\n";
    return 0;
}
