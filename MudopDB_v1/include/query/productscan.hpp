#ifndef PRODUCTSCAN_HPP
#define PRODUCTSCAN_HPP

#include "query/scan.hpp"
#include <memory>

class ProductScan : public Scan {
public:
    ProductScan(std::unique_ptr<Scan> s1, std::unique_ptr<Scan> s2);

    void before_first() override;
    bool next() override;
    int get_int(const std::string& fldname) override;
    std::string get_string(const std::string& fldname) override;
    Constant get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    void close() override;

private:
    std::unique_ptr<Scan> s1_;
    std::unique_ptr<Scan> s2_;
};

#endif // PRODUCTSCAN_HPP
