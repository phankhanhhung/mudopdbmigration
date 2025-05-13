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

    void before_first() override;
    bool next() override;
    int get_int(const std::string& fldname) override;
    std::string get_string(const std::string& fldname) override;
    Constant get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    void close() override;

private:
    std::unique_ptr<Scan> s_;
    std::vector<std::string> fieldlist_;
};

#endif // PROJECTSCAN_HPP
