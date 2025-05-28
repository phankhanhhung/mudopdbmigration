#ifndef MODIFYDATA_HPP
#define MODIFYDATA_HPP

#include "query/expression.hpp"
#include "query/predicate.hpp"
#include <string>

namespace parse {

class ModifyData {
public:
    ModifyData(const std::string& tblname,
               const std::string& fldname,
               const Expression& newval,
               const Predicate& pred);

    std::string table_name() const;
    std::string target_field() const;
    Expression new_value() const;
    Predicate pred() const;

private:
    std::string tblname_;
    std::string fldname_;
    Expression newval_;
    Predicate pred_;
};

} // namespace parse

#endif // MODIFYDATA_HPP
