#ifndef COUNTFN_HPP
#define COUNTFN_HPP

#include "materialize/aggregationfn.hpp"

namespace materialize {

class CountFn : public AggregationFn {
public:
    explicit CountFn(const std::string& fldname);

    void process_first(Scan& s) override;
    void process_next(Scan& s) override;
    std::string field_name() const override;
    std::optional<Constant> value() const override;

private:
    std::string fldname_;
    size_t count_;
};

} // namespace materialize

#endif // COUNTFN_HPP
