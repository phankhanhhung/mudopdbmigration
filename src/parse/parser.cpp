#include "parse/parser.hpp"

namespace parse {

Parser::Parser(const std::string& s) : lex_(s) {}

std::string Parser::field() {
    return lex_.eat_id();
}

Constant Parser::constant() {
    if (lex_.match_string_constant()) {
        return Constant::with_string(lex_.eat_string_constant());
    }
    return Constant::with_int(lex_.eat_int_constant());
}

Expression Parser::expression() {
    if (lex_.match_id()) {
        return Expression::with_string(field());
    }
    return Expression::with_constant(constant());
}

Term Parser::term() {
    Expression lhs = expression();
    lex_.eat_delim('=');
    Expression rhs = expression();
    return Term(lhs, rhs);
}

Predicate Parser::predicate() {
    Predicate pred = Predicate::with_term(term());
    if (lex_.match_keyword("and")) {
        lex_.eat_keyword("and");
        pred.conjoin_with(predicate());
    }
    return pred;
}

QueryData Parser::query() {
    lex_.eat_keyword("select");
    auto fields = select_list();
    lex_.eat_keyword("from");
    auto tables = table_list();
    Predicate pred;
    if (lex_.match_keyword("where")) {
        lex_.eat_keyword("where");
        pred = predicate();
    }
    return QueryData(fields, tables, pred);
}

std::vector<std::string> Parser::select_list() {
    std::vector<std::string> l;
    l.push_back(field());
    if (lex_.match_delim(',')) {
        lex_.eat_delim(',');
        auto rest = select_list();
        l.insert(l.end(), rest.begin(), rest.end());
    }
    return l;
}

std::vector<std::string> Parser::table_list() {
    std::vector<std::string> l;
    l.push_back(lex_.eat_id());
    if (lex_.match_delim(',')) {
        lex_.eat_delim(',');
        auto rest = table_list();
        l.insert(l.end(), rest.begin(), rest.end());
    }
    return l;
}

Object Parser::update_cmd() {
    if (lex_.match_keyword("insert")) {
        return insert_cmd();
    } else if (lex_.match_keyword("delete")) {
        return delete_cmd();
    } else if (lex_.match_keyword("update")) {
        return modify_cmd();
    }
    return create_cmd();
}

Object Parser::create_cmd() {
    lex_.eat_keyword("create");
    if (lex_.match_keyword("table")) {
        return create_table();
    } else if (lex_.match_keyword("view")) {
        return create_view();
    }
    return create_index();
}

DeleteData Parser::delete_cmd() {
    lex_.eat_keyword("delete");
    lex_.eat_keyword("from");
    std::string tblname = lex_.eat_id();
    Predicate pred;
    if (lex_.match_keyword("where")) {
        lex_.eat_keyword("where");
        pred = predicate();
    }
    return DeleteData(tblname, pred);
}

InsertData Parser::insert_cmd() {
    lex_.eat_keyword("insert");
    lex_.eat_keyword("into");
    std::string tblname = lex_.eat_id();
    lex_.eat_delim('(');
    auto flds = field_list();
    lex_.eat_delim(')');
    lex_.eat_keyword("values");
    lex_.eat_delim('(');
    auto vals = const_list();
    lex_.eat_delim(')');
    return InsertData(tblname, flds, vals);
}

std::vector<std::string> Parser::field_list() {
    std::vector<std::string> l;
    l.push_back(field());
    if (lex_.match_delim(',')) {
        lex_.eat_delim(',');
        auto rest = field_list();
        l.insert(l.end(), rest.begin(), rest.end());
    }
    return l;
}

std::vector<Constant> Parser::const_list() {
    std::vector<Constant> l;
    l.push_back(constant());
    if (lex_.match_delim(',')) {
        lex_.eat_delim(',');
        auto rest = const_list();
        l.insert(l.end(), rest.begin(), rest.end());
    }
    return l;
}

ModifyData Parser::modify_cmd() {
    lex_.eat_keyword("update");
    std::string tblname = lex_.eat_id();
    lex_.eat_keyword("set");
    std::string fldname = field();
    lex_.eat_delim('=');
    Expression newval = expression();
    Predicate pred;
    if (lex_.match_keyword("where")) {
        lex_.eat_keyword("where");
        pred = predicate();
    }
    return ModifyData(tblname, fldname, newval, pred);
}

CreateTableData Parser::create_table() {
    lex_.eat_keyword("table");
    std::string tblname = lex_.eat_id();
    lex_.eat_delim('(');
    record::Schema sch = field_defs();
    lex_.eat_delim(')');
    return CreateTableData(tblname, sch);
}

record::Schema Parser::field_defs() {
    record::Schema schema = field_def();
    if (lex_.match_delim(',')) {
        lex_.eat_delim(',');
        record::Schema schema2 = field_defs();
        schema.add_all(schema2);
    }
    return schema;
}

record::Schema Parser::field_def() {
    std::string fldname = field();
    return field_type(fldname);
}

record::Schema Parser::field_type(const std::string& fldname) {
    record::Schema schema;
    if (lex_.match_keyword("int")) {
        lex_.eat_keyword("int");
        schema.add_int_field(fldname);
    } else {
        lex_.eat_keyword("varchar");
        lex_.eat_delim('(');
        int32_t str_len = lex_.eat_int_constant();
        lex_.eat_delim(')');
        schema.add_string_field(fldname, static_cast<size_t>(str_len));
    }
    return schema;
}

CreateViewData Parser::create_view() {
    lex_.eat_keyword("view");
    std::string viewname = lex_.eat_id();
    lex_.eat_keyword("as");
    QueryData qd = query();
    return CreateViewData(viewname, qd);
}

CreateIndexData Parser::create_index() {
    lex_.eat_keyword("index");
    std::string idxname = lex_.eat_id();
    lex_.eat_keyword("on");
    std::string tblname = lex_.eat_id();
    lex_.eat_delim('(');
    std::string fldname = field();
    lex_.eat_delim(')');
    return CreateIndexData(idxname, tblname, fldname);
}

} // namespace parse
