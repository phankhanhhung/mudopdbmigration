#ifndef MAXFN_HPP
#define MAXFN_HPP

#include "materialize/aggregationfn.hpp"

namespace materialize {

class MaxFn : public AggregationFn {
public:
    explicit MaxFn(const std::string& fldname);

    void process_first(Scan& s) override;
    void process_next(Scan& s) override;
    std::string field_name() const override;
    std::optional<Constant> value() const override;

private:
    std::string fldname_;
    std::optional<Constant> val_;
};

} // namespace materialize

#endif // MAXFN_HPP
