#ifndef QUERYDATA_HPP
#define QUERYDATA_HPP

#include "query/predicate.hpp"
#include <string>
#include <vector>

namespace parse {

class QueryData {
public:
    QueryData(const std::vector<std::string>& fields,
              const std::vector<std::string>& tables,
              const Predicate& pred);

    std::vector<std::string> fields() const;
    std::vector<std::string> tables() const;
    Predicate pred() const;
    std::string to_string() const;

private:
    std::vector<std::string> fields_;
    std::vector<std::string> tables_;
    Predicate pred_;
};

} // namespace parse

#endif // QUERYDATA_HPP
