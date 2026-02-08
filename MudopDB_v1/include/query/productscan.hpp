#ifndef PRODUCTSCAN_HPP
#define PRODUCTSCAN_HPP

#include "query/scan.hpp"
#include <memory>

/**
 * ProductScan computes the Cartesian product of two scans.
 *
 * Corresponds to ProductScan in Rust (NMDB2/src/query/productscan.rs)
 */
class ProductScan : public Scan {
public:
    ProductScan(std::unique_ptr<Scan> s1, std::unique_ptr<Scan> s2);

    DbResult<void> before_first() override;
    DbResult<bool> next() override;
    DbResult<int> get_int(const std::string& fldname) override;
    DbResult<std::string> get_string(const std::string& fldname) override;
    DbResult<Constant> get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    DbResult<void> close() override;

private:
    std::unique_ptr<Scan> s1_;
    std::unique_ptr<Scan> s2_;
};

#endif // PRODUCTSCAN_HPP
