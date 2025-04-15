#ifndef TABLESCAN_HPP
#define TABLESCAN_HPP

#include "record/recordpage.hpp"
#include "record/layout.hpp"
#include "record/rid.hpp"
#include "query/updatescan.hpp"
#include "query/constant.hpp"
#include <memory>
#include <optional>
#include <string>

namespace tx {
class Transaction;
}

namespace record {

class TableScan : public UpdateScan {
public:
    TableScan(std::shared_ptr<tx::Transaction> tx,
              const std::string& tablename,
              const Layout& layout);

    // Scan interface implementation
    void before_first() override;
    bool next() override;
    int get_int(const std::string& fldname) override;
    std::string get_string(const std::string& fldname) override;
    Constant get_val(const std::string& fldname) override;
    bool has_field(const std::string& fldname) const override;
    void close() override;

    // UpdateScan interface implementation
    void set_val(const std::string& fldname, const Constant& val) override;
    void set_int(const std::string& fldname, int32_t val) override;
    void set_string(const std::string& fldname, const std::string& val) override;
    void insert() override;
    void delete_record() override;
    std::optional<RID> get_rid() const override;
    void move_to_rid(const RID& rid) override;

private:
    void move_to_block(int32_t blknum);
    void move_to_new_block();
    bool at_last_block() const;

    std::shared_ptr<tx::Transaction> tx_;
    Layout layout_;
    std::unique_ptr<RecordPage> rp_;
    std::string filename_;
    std::optional<size_t> currentslot_;
};

} // namespace record

#endif // TABLESCAN_HPP
