#ifndef MULTIBUFFERPRODUCTPLAN_HPP
#define MULTIBUFFERPRODUCTPLAN_HPP

#include "plan/plan.hpp"
#include "record/schema.hpp"
#include <memory>

namespace tx { class Transaction; }

namespace multibuffer {

/**
 * Plan for multi-buffer product (large Cartesian products).
 * Materializes RHS into temp table, processes in chunks.
 *
 * Corresponds to MultibufferProductPlan in Rust (NMDB2/src/multibuffer/multibufferproductplan.rs)
 */
class MultibufferProductPlan : public Plan {
public:
    MultibufferProductPlan(std::shared_ptr<tx::Transaction> tx,
                           std::shared_ptr<Plan> lhs,
                           std::shared_ptr<Plan> rhs);

    std::unique_ptr<Scan> open() override;
    size_t blocks_accessed() const override;
    size_t records_output() const override;
    size_t distinct_values(const std::string& fldname) const override;
    std::shared_ptr<record::Schema> schema() const override;

private:
    std::shared_ptr<tx::Transaction> tx_;
    std::shared_ptr<Plan> lhs_;
    std::shared_ptr<Plan> rhs_;
    std::shared_ptr<record::Schema> schema_;
};

} // namespace multibuffer

#endif // MULTIBUFFERPRODUCTPLAN_HPP
