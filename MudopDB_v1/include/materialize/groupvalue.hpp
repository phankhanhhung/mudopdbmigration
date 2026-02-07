#ifndef GROUPVALUE_HPP
#define GROUPVALUE_HPP

#include "query/constant.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

class Scan;

namespace materialize {

class GroupValue {
public:
    GroupValue(Scan& s, const std::vector<std::string>& fields);

    std::optional<Constant> get_val(const std::string& fldname) const;
    bool operator==(const GroupValue& other) const;
    bool operator!=(const GroupValue& other) const;

private:
    std::unordered_map<std::string, Constant> vals_;
};

} // namespace materialize

#endif // GROUPVALUE_HPP
