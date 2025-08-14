#ifndef RECORDCOMPARATOR_HPP
#define RECORDCOMPARATOR_HPP

#include <string>
#include <vector>

class Scan;

namespace materialize {

class RecordComparator {
public:
    explicit RecordComparator(const std::vector<std::string>& fields);

    int compare(Scan& s1, Scan& s2) const;

private:
    std::vector<std::string> fields_;
};

} // namespace materialize

#endif // RECORDCOMPARATOR_HPP
