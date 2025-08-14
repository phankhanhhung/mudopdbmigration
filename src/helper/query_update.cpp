#include "helper/query_update.hpp"
#include "api/statement.hpp"
#include "api/result_set.hpp"
#include "api/metadata.hpp"
#include <iostream>
#include <memory>
#include <iomanip> 

void do_query(Statement *stmt, const std::string &cmd) {
    std::cout << "Executing query: " << cmd << std::endl;
    // Implement query logic here.
    std::unique_ptr<ResultSet> rs = stmt->execute_query(cmd);
    // Use the interface's declared return type (pointer, ref-wrapped, or unique_ptr)
    auto md = rs->get_meta_data();
    size_t numcols = md->get_column_count();
    size_t totalwidth = 0;

    for (int i = 1; i <= numcols; i++) {
      std::string fldname = md->get_column_name(i);
      size_t width = md->get_column_display_size(i);
      totalwidth += width;
    }
    std::cout << std::endl;

    while (rs->next()) {
      for (int i = 1; i <= numcols; i++) {
        std::string fldname = md->get_column_name(i);
        size_t width = md->get_column_display_size(i);

        switch (md->get_column_type(i)) {
          case record::Type::INTEGER: {
            // TODO: render integer value with width
            size_t ival = rs->get_int(fldname);
            std::cout << std::setw(width) << ival;
            break;
          }
          case record::Type::VARCHAR: {
            // TODO: render varchar value with width
            std::string sval = rs->get_string(fldname);
            std::cout << std::setw(width) << sval;
            break;
          }
          default:
            break;
        }
      }
      std::cout << std::endl;
    }
    rs->close();
}

void do_update(Statement *stmt, const std::string &cmd) {
    std::cout << "Executing update: " << cmd << std::endl;
    size_t howmany = stmt->execute_update(cmd);
    std::cout << howmany << " records processed\n";
    // Implement update logic here.
}