#ifndef DELETEDATA_HPP
#define DELETEDATA_HPP

#include "query/predicate.hpp"
#include <string>

namespace parse {

class DeleteData {
public:
    DeleteData(const std::string& tblname, const Predicate& pred);

    std::string table_name() const;
    Predicate pred() const;

private:
    std::string tblname_;
    Predicate pred_;
};

} // namespace parse

#endif // DELETEDATA_HPP
