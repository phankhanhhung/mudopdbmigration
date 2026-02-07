#ifndef INDEXSELECTSCAN_HPP
#define INDEXSELECTSCAN_HPP

#include "index/index.hpp"
#include "query/scan.hpp"
#include "query/constant.hpp"
#include "record/tablescan.hpp"
#include <memory>

namespace index {

/**
 * Scan that uses an index for selection (equality lookup).
 *
 * Corresponds to IndexSelectScan in Rust (NMDB2/src/index/query/indexselectscan.rs)
 */
class IndexSelectScan : public Scan {
public:
    IndexSelectScan(std::unique_ptr<record::TableScan> ts,
                    std::unique_ptr<::Index> idx,
                    const Constant& val);

    void before_first() override;
    bool next() override;
    int32_t get_int(const std::string& fldname) override;
    std::string get_string(const std::string& fldname) override;
    Constant get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    void close() override;

private:
    std::unique_ptr<record::TableScan> ts_;
    std::unique_ptr<::Index> idx_;
    Constant val_;
};

} // namespace index

#endif // INDEXSELECTSCAN_HPP
