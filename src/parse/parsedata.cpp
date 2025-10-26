/**
 * Parse data structures implementation.
 * Concrete representations of parsed SQL statements.
 */

#include "parse/querydata.hpp"
#include "parse/insertdata.hpp"
#include "parse/deletedata.hpp"
#include "parse/modifydata.hpp"
#include "parse/createtabledata.hpp"
#include "parse/createviewdata.hpp"
#include "parse/createindexdata.hpp"

namespace parse {

// QueryData
QueryData::QueryData(const std::vector<std::string>& fields,
                     const std::vector<std::string>& tables,
                     const Predicate& pred)
    : fields_(fields), tables_(tables), pred_(pred) {}

std::vector<std::string> QueryData::fields() const { return fields_; }
std::vector<std::string> QueryData::tables() const { return tables_; }
Predicate QueryData::pred() const { return pred_; }

std::string QueryData::to_string() const {
    std::string result = "select ";
    for (size_t i = 0; i < fields_.size(); i++) {
        if (i > 0) result += ", ";
        result += fields_[i];
    }
    result += " from ";
    for (size_t i = 0; i < tables_.size(); i++) {
        if (i > 0) result += ", ";
        result += tables_[i];
    }
    std::string predstr = pred_.to_string();
    if (!predstr.empty()) {
        result += " where " + predstr;
    }
    return result;
}

// InsertData
InsertData::InsertData(const std::string& tblname,
                       const std::vector<std::string>& flds,
                       const std::vector<Constant>& vals)
    : tblname_(tblname), flds_(flds), vals_(vals) {}

std::string InsertData::table_name() const { return tblname_; }
std::vector<std::string> InsertData::fields() const { return flds_; }
std::vector<Constant> InsertData::vals() const { return vals_; }

// DeleteData
DeleteData::DeleteData(const std::string& tblname, const Predicate& pred)
    : tblname_(tblname), pred_(pred) {}

std::string DeleteData::table_name() const { return tblname_; }
Predicate DeleteData::pred() const { return pred_; }

// ModifyData
ModifyData::ModifyData(const std::string& tblname,
                       const std::string& fldname,
                       const Expression& newval,
                       const Predicate& pred)
    : tblname_(tblname), fldname_(fldname), newval_(newval), pred_(pred) {}

std::string ModifyData::table_name() const { return tblname_; }
std::string ModifyData::target_field() const { return fldname_; }
Expression ModifyData::new_value() const { return newval_; }
Predicate ModifyData::pred() const { return pred_; }

// CreateTableData
CreateTableData::CreateTableData(const std::string& tblname, const record::Schema& sch)
    : tblname_(tblname), sch_(std::make_shared<record::Schema>(sch)) {}

std::string CreateTableData::table_name() const { return tblname_; }
std::shared_ptr<record::Schema> CreateTableData::new_schema() const { return sch_; }

// CreateViewData
CreateViewData::CreateViewData(const std::string& viewname, const QueryData& qd)
    : viewname_(viewname), qd_(qd) {}

std::string CreateViewData::view_name() const { return viewname_; }
std::string CreateViewData::view_def() const { return qd_.to_string(); }

// CreateIndexData
CreateIndexData::CreateIndexData(const std::string& idxname,
                                 const std::string& tblname,
                                 const std::string& fldname)
    : idxname_(idxname), tblname_(tblname), fldname_(fldname) {}

std::string CreateIndexData::index_name() const { return idxname_; }
std::string CreateIndexData::table_name() const { return tblname_; }
std::string CreateIndexData::field_name() const { return fldname_; }

} // namespace parse
