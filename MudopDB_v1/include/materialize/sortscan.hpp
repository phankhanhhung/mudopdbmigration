#ifndef SORTSCAN_HPP
#define SORTSCAN_HPP

#include "query/scan.hpp"
#include "record/tablescan.hpp"
#include "record/rid.hpp"
#include "materialize/temptable.hpp"
#include "materialize/recordcomparator.hpp"
#include <memory>
#include <vector>
#include <optional>
#include <array>

namespace materialize {

class SortScan : public Scan {
public:
    SortScan(const std::vector<std::shared_ptr<TempTable>>& runs,
             const RecordComparator& comp);

    DbResult<void> before_first() override;
    DbResult<bool> next() override;
    DbResult<int> get_int(const std::string& fldname) override;
    DbResult<std::string> get_string(const std::string& fldname) override;
    DbResult<Constant> get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    DbResult<void> close() override;

    void save_position();
    void restore_position();

private:
    record::TableScan& current_scan();

    std::unique_ptr<record::TableScan> s1_;
    std::unique_ptr<record::TableScan> s2_;
    RecordComparator comp_;
    std::optional<int> currentidx_;
    bool hasmore1_;
    bool hasmore2_;
    std::array<std::optional<record::RID>, 2> savedposition_;
};

} // namespace materialize

#endif // SORTSCAN_HPP
