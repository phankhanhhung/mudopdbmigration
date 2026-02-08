#ifndef PROJECTSCAN_HPP
#define PROJECTSCAN_HPP

#include "query/scan.hpp"
#include <memory>
#include <vector>
#include <string>

/**
 * ProjectScan restricts access to a subset of fields.
 *
 * Corresponds to ProjectScan in Rust (NMDB2/src/query/projectscan.rs)
 */
class ProjectScan : public Scan {
public:
    ProjectScan(std::unique_ptr<Scan> s, const std::vector<std::string>& fieldlist);

    DbResult<void> before_first() override;
    DbResult<bool> next() override;
    DbResult<int> get_int(const std::string& fldname) override;
    DbResult<std::string> get_string(const std::string& fldname) override;
    DbResult<Constant> get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    DbResult<void> close() override;

private:
    std::unique_ptr<Scan> s_;
    std::vector<std::string> fieldlist_;
};

#endif // PROJECTSCAN_HPP
