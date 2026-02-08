#ifndef GROUPBYSCAN_HPP
#define GROUPBYSCAN_HPP

#include "query/scan.hpp"
#include "materialize/aggregationfn.hpp"
#include "materialize/groupvalue.hpp"
#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace materialize {

class GroupByScan : public Scan {
public:
    GroupByScan(std::unique_ptr<Scan> s,
                const std::vector<std::string>& groupfields,
                std::vector<std::unique_ptr<AggregationFn>> aggfns);

    DbResult<void> before_first() override;
    DbResult<bool> next() override;
    DbResult<int> get_int(const std::string& fldname) override;
    DbResult<std::string> get_string(const std::string& fldname) override;
    DbResult<Constant> get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    DbResult<void> close() override;

private:
    std::unique_ptr<Scan> s_;
    std::vector<std::string> groupfields_;
    std::vector<std::unique_ptr<AggregationFn>> aggfns_;
    std::optional<GroupValue> groupval_;
    bool moregroups_;
};

} // namespace materialize

#endif // GROUPBYSCAN_HPP
