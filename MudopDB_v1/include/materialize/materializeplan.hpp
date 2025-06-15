#ifndef MATERIALIZEPLAN_HPP
#define MATERIALIZEPLAN_HPP

#include "plan/plan.hpp"
#include <memory>

namespace tx { class Transaction; }

namespace materialize {

class MaterializePlan : public Plan {
public:
    MaterializePlan(std::shared_ptr<tx::Transaction> tx, std::shared_ptr<Plan> srcplan);

    std::unique_ptr<Scan> open() override;
    size_t blocks_accessed() const override;
    size_t records_output() const override;
    size_t distinct_values(const std::string& fldname) const override;
    std::shared_ptr<record::Schema> schema() const override;

private:
    std::shared_ptr<Plan> srcplan_;
    std::shared_ptr<tx::Transaction> tx_;
};

} // namespace materialize

#endif // MATERIALIZEPLAN_HPP
