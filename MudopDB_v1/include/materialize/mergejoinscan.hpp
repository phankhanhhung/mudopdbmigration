#ifndef MERGEJOINSCAN_HPP
#define MERGEJOINSCAN_HPP

#include "query/scan.hpp"
#include "query/constant.hpp"
#include "materialize/sortscan.hpp"
#include <memory>
#include <string>
#include <optional>

namespace materialize {

class MergeJoinScan : public Scan {
public:
    MergeJoinScan(std::unique_ptr<Scan> s1, std::unique_ptr<SortScan> s2,
                  const std::string& fldname1, const std::string& fldname2);

    void before_first() override;
    bool next() override;
    int get_int(const std::string& fldname) override;
    std::string get_string(const std::string& fldname) override;
    Constant get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    void close() override;

private:
    std::unique_ptr<Scan> s1_;
    std::unique_ptr<SortScan> s2_;
    std::string fldname1_;
    std::string fldname2_;
    std::optional<Constant> joinval_;
};

} // namespace materialize

#endif // MERGEJOINSCAN_HPP
