#ifndef CHUNKSCAN_HPP
#define CHUNKSCAN_HPP

#include "query/scan.hpp"
#include "query/constant.hpp"
#include "record/layout.hpp"
#include "record/recordpage.hpp"
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace tx { class Transaction; }

namespace multibuffer {

/**
 * Scan over a contiguous range of blocks, keeping all in memory.
 *
 * Corresponds to ChunkScan in Rust (NMDB2/src/multibuffer/chunkscan.rs)
 */
class ChunkScan : public Scan {
public:
    ChunkScan(std::shared_ptr<tx::Transaction> tx,
              const std::string& filename,
              const record::Layout& layout,
              size_t startbnum,
              size_t endbnum);

    void before_first() override;
    bool next() override;
    int32_t get_int(const std::string& fldname) override;
    std::string get_string(const std::string& fldname) override;
    Constant get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    void close() override;

private:
    void move_to_block(size_t blknum);

    std::vector<record::RecordPage> buffs_;
    std::shared_ptr<tx::Transaction> tx_;
    std::string filename_;
    record::Layout layout_;
    size_t startbnum_;
    size_t endbnum_;
    size_t currentbnum_;
    size_t rpidx_;
    std::optional<size_t> currentslot_;
};

} // namespace multibuffer

#endif // CHUNKSCAN_HPP
