#ifndef CREATEINDEXDATA_HPP
#define CREATEINDEXDATA_HPP

#include <string>

namespace parse {

class CreateIndexData {
public:
    CreateIndexData(const std::string& idxname,
                    const std::string& tblname,
                    const std::string& fldname);

    std::string index_name() const;
    std::string table_name() const;
    std::string field_name() const;

private:
    std::string idxname_;
    std::string tblname_;
    std::string fldname_;
};

} // namespace parse

#endif // CREATEINDEXDATA_HPP
