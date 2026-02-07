#ifndef INDEXJOINSCAN_HPP
#define INDEXJOINSCAN_HPP

#include "index/index.hpp"
#include "query/scan.hpp"
#include "query/constant.hpp"
#include "record/tablescan.hpp"
#include <memory>
#include <string>

namespace index {

/**
 * Scan that uses an index for nested-loop join.
 *
 * Corresponds to IndexJoinScan in Rust (NMDB2/src/index/query/indexjoinscan.rs)
 */
class IndexJoinScan : public Scan {
public:
    IndexJoinScan(std::unique_ptr<Scan> lhs,
                  std::unique_ptr<::Index> idx,
                  const std::string& joinfield,
                  std::unique_ptr<record::TableScan> rhs);

    void before_first() override;
    bool next() override;
    int32_t get_int(const std::string& fldname) override;
    std::string get_string(const std::string& fldname) override;
    Constant get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    void close() override;

private:
    void reset_index();

    std::unique_ptr<Scan> lhs_;
    std::unique_ptr<::Index> idx_;
    std::string joinfield_;
    std::unique_ptr<record::TableScan> rhs_;
};

} // namespace index

#endif // INDEXJOINSCAN_HPP
