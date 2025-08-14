#ifndef TABLEPLAN_HPP
#define TABLEPLAN_HPP

#include "plan/plan.hpp"
#include "record/layout.hpp"
#include "metadata/statinfo.hpp"
#include <memory>
#include <string>

namespace tx { class Transaction; }
namespace metadata { class MetadataMgr; }

class TablePlan : public Plan {
public:
    TablePlan(std::shared_ptr<tx::Transaction> tx,
              const std::string& tblname,
              std::shared_ptr<metadata::MetadataMgr> mdm);

    std::unique_ptr<Scan> open() override;
    size_t blocks_accessed() const override;
    size_t records_output() const override;
    size_t distinct_values(const std::string& fldname) const override;
    std::shared_ptr<record::Schema> schema() const override;

private:
    std::string tblname_;
    std::shared_ptr<tx::Transaction> tx_;
    record::Layout layout_;
    metadata::StatInfo si_;
};

#endif // TABLEPLAN_HPP
