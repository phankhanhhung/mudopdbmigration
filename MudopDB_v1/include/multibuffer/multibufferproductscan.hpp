#ifndef MULTIBUFFERPRODUCTSCAN_HPP
#define MULTIBUFFERPRODUCTSCAN_HPP

#include "query/scan.hpp"
#include "query/constant.hpp"
#include "record/layout.hpp"
#include <memory>
#include <string>

namespace tx { class Transaction; }

namespace multibuffer {

/**
 * Scan for multi-buffer product. Processes RHS in chunks.
 *
 * Corresponds to MultibufferProductScan in Rust (NMDB2/src/multibuffer/multibufferproductscan.rs)
 */
class MultibufferProductScan : public Scan {
public:
    MultibufferProductScan(std::shared_ptr<tx::Transaction> tx,
                           std::unique_ptr<Scan> lhsscan,
                           const std::string& tblname,
                           const record::Layout& layout);

    void before_first() override;
    bool next() override;
    int32_t get_int(const std::string& fldname) override;
    std::string get_string(const std::string& fldname) override;
    Constant get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    void close() override;

private:
    bool use_next_chunk();

    std::shared_ptr<tx::Transaction> tx_;
    std::unique_ptr<Scan> lhsscan_;
    std::unique_ptr<Scan> rhsscan_;
    std::string filename_;
    record::Layout layout_;
    size_t chunksize_;
    size_t nextblknum_;
    size_t filesize_;
};

} // namespace multibuffer

#endif // MULTIBUFFERPRODUCTSCAN_HPP
