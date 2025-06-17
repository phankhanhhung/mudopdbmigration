#include "materialize/countfn.hpp"
#include "materialize/maxfn.hpp"
#include "materialize/groupvalue.hpp"
#include "query/scan.hpp"

namespace materialize {

// CountFn
CountFn::CountFn(const std::string& fldname) : fldname_(fldname), count_(0) {}

void CountFn::process_first(Scan& /*s*/) { count_ = 1; }
void CountFn::process_next(Scan& /*s*/) { count_++; }
std::string CountFn::field_name() const { return "countof" + fldname_; }
std::optional<Constant> CountFn::value() const {
    return Constant::with_int(static_cast<int>(count_));
}

// MaxFn
MaxFn::MaxFn(const std::string& fldname) : fldname_(fldname) {}

void MaxFn::process_first(Scan& s) { val_ = s.get_val(fldname_); }
void MaxFn::process_next(Scan& s) {
    Constant newval = s.get_val(fldname_);
    if (val_.has_value() && newval > val_.value()) {
        val_ = newval;
    }
}
std::string MaxFn::field_name() const { return "maxof" + fldname_; }
std::optional<Constant> MaxFn::value() const { return val_; }

// GroupValue
GroupValue::GroupValue(Scan& s, const std::vector<std::string>& fields) {
    for (const auto& fldname : fields) {
        vals_.emplace(fldname, s.get_val(fldname));
    }
}

std::optional<Constant> GroupValue::get_val(const std::string& fldname) const {
    auto it = vals_.find(fldname);
    if (it != vals_.end()) return it->second;
    return std::nullopt;
}

bool GroupValue::operator==(const GroupValue& other) const {
    return vals_ == other.vals_;
}

bool GroupValue::operator!=(const GroupValue& other) const {
    return !(*this == other);
}

} // namespace materialize
