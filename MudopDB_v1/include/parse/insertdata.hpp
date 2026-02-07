#ifndef INSERTDATA_HPP
#define INSERTDATA_HPP

#include "query/constant.hpp"
#include <string>
#include <vector>

namespace parse {

class InsertData {
public:
    InsertData(const std::string& tblname,
               const std::vector<std::string>& flds,
               const std::vector<Constant>& vals);

    std::string table_name() const;
    std::vector<std::string> fields() const;
    std::vector<Constant> vals() const;

private:
    std::string tblname_;
    std::vector<std::string> flds_;
    std::vector<Constant> vals_;
};

} // namespace parse

#endif // INSERTDATA_HPP
