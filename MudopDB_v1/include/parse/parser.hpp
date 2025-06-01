#ifndef PARSER_HPP
#define PARSER_HPP

#include "parse/lexer.hpp"
#include "parse/querydata.hpp"
#include "parse/insertdata.hpp"
#include "parse/deletedata.hpp"
#include "parse/modifydata.hpp"
#include "parse/createtabledata.hpp"
#include "parse/createviewdata.hpp"
#include "parse/createindexdata.hpp"
#include "query/expression.hpp"
#include "query/predicate.hpp"
#include "query/constant.hpp"
#include "record/schema.hpp"
#include <variant>
#include <string>
#include <vector>

namespace parse {

using Object = std::variant<InsertData, DeleteData, ModifyData,
                            CreateTableData, CreateViewData, CreateIndexData>;

/**
 * Recursive descent SQL parser.
 *
 * Corresponds to Parser in Rust (NMDB2/src/parse/parser.rs)
 */
class Parser {
public:
    explicit Parser(const std::string& s);

    // Public parse methods
    std::string field();
    Constant constant();
    Expression expression();
    Term term();
    Predicate predicate();
    QueryData query();
    Object update_cmd();

    // Specific update commands
    DeleteData delete_cmd();
    InsertData insert_cmd();
    ModifyData modify_cmd();
    CreateTableData create_table();
    CreateViewData create_view();
    CreateIndexData create_index();

private:
    std::vector<std::string> select_list();
    std::vector<std::string> table_list();
    std::vector<std::string> field_list();
    std::vector<Constant> const_list();
    Object create_cmd();
    record::Schema field_defs();
    record::Schema field_def();
    record::Schema field_type(const std::string& fldname);

    Lexer lex_;
};

} // namespace parse

#endif // PARSER_HPP
