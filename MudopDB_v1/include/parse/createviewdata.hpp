#ifndef CREATEVIEWDATA_HPP
#define CREATEVIEWDATA_HPP

#include "parse/querydata.hpp"
#include <string>

namespace parse {

class CreateViewData {
public:
    CreateViewData(const std::string& viewname, const QueryData& qd);

    std::string view_name() const;
    std::string view_def() const;

private:
    std::string viewname_;
    QueryData qd_;
};

} // namespace parse

#endif // CREATEVIEWDATA_HPP
