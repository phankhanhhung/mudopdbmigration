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

int main() {
    std::string db_dir = "/tmp/mudopdb_persistent";

    if (!fs::exists(db_dir)) {
        std::cout << "ERROR: Database not found. Run writer first.\n";
        return 1;
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

    scan.before_first().value();
    int count = 0;
    int total_balance = 0;

    while (scan.next().value()) {
        int id = scan.get_int("id").value();
        std::string name = scan.get_string("name").value();
        int balance = scan.get_int("balance").value();
        std::cout << std::setw(10) << id
                  << std::setw(25) << name
                  << std::setw(15) << balance << "\n";
        count++;
        total_balance += balance;
    }

    std::cout << "Total: " << count << " records, $" << total_balance << "\n";

    scan.close().value();
    tx->commit();

    return (count == 5 && total_balance == 44450) ? 0 : 1;
}
