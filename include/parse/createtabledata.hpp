#ifndef CREATETABLEDATA_HPP
#define CREATETABLEDATA_HPP

#include "record/schema.hpp"
#include <memory>
#include <string>

namespace parse {

class CreateTableData {
public:
    CreateTableData(const std::string& tblname, const record::Schema& sch);

    std::string table_name() const;
    std::shared_ptr<record::Schema> new_schema() const;

private:
    std::string tblname_;
    std::shared_ptr<record::Schema> sch_;
};

} // namespace parse

#endif // CREATETABLEDATA_HPP
