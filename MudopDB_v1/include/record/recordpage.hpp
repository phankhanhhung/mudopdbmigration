#ifndef RECORDPAGE_HPP
#define RECORDPAGE_HPP

#include "record/layout.hpp"
#include "file/blockid.hpp"
#include <memory>
#include <optional>
#include <cstdint>

namespace tx {
class Transaction;
}

namespace record {

class RecordPage {
public:
    RecordPage(std::shared_ptr<tx::Transaction> tx,
               const file::BlockId& blk,
               const Layout& layout);

    int32_t get_int(size_t slot, const std::string& fldname);
    std::string get_string(size_t slot, const std::string& fldname);
    void set_int(size_t slot, const std::string& fldname, int32_t val);
    void set_string(size_t slot, const std::string& fldname, const std::string& val);

    void delete_record(size_t slot);
    void format();

    std::optional<size_t> next_after(std::optional<size_t> slot);
    std::optional<size_t> insert_after(std::optional<size_t> slot);

    const file::BlockId& block() const;

private:
    enum class Flag : int32_t {
        EMPTY = 0,
        USED = 1
    };

    void set_flag(size_t slot, Flag flag);
    std::optional<size_t> search_after(std::optional<size_t> slot, Flag flag);
    bool is_valid_slot(size_t slot) const;
    size_t offset(size_t slot) const;

    std::shared_ptr<tx::Transaction> tx_;
    file::BlockId blk_;
    Layout layout_;
};

} // namespace record

#endif // RECORDPAGE_HPP
