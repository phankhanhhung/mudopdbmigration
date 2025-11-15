/**
 * RecordComparator implementation.
 * Compares records by a list of field names for sorting.
 */

#include "materialize/recordcomparator.hpp"
#include "query/scan.hpp"

namespace materialize {

RecordComparator::RecordComparator(const std::vector<std::string>& fields)
    : fields_(fields) {}

int RecordComparator::compare(Scan& s1, Scan& s2) const {
    for (const auto& fldname : fields_) {
        Constant val1 = s1.get_val(fldname);
        Constant val2 = s2.get_val(fldname);
        if (val1 > val2) return 1;
        if (val1 < val2) return -1;
    }
    return 0;
}

} // namespace materialize
