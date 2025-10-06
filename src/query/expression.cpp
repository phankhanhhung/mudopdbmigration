/**
 * Expression implementation.
 * Represents either a field reference or a constant value in a SQL expression.
 */

#include "query/expression.hpp"
#include "query/scan.hpp"
#include "record/schema.hpp"
#include <stdexcept>

Expression Expression::with_constant(const Constant& val) {
    Expression e;
    e.val_ = val;
    return e;
}

Expression Expression::with_string(const std::string& fldname) {
    Expression e;
    e.fldname_ = fldname;
    return e;
}

Constant Expression::evaluate(Scan& s) const {
    if (val_.has_value()) {
        return val_.value();
    }
    if (fldname_.has_value()) {
        return s.get_val(fldname_.value());
    }
    throw std::runtime_error("Expression: no value or field name");
}

std::optional<Constant> Expression::as_constant() const {
    return val_;
}

std::optional<std::string> Expression::as_field_name() const {
    return fldname_;
}

bool Expression::applies_to(const record::Schema& sch) const {
    if (val_.has_value()) {
        return true;
    }
    if (fldname_.has_value()) {
        return sch.has_field(fldname_.value());
    }
    return false;
}

std::string Expression::to_string() const {
    if (val_.has_value()) {
        return val_.value().to_string();
    }
    if (fldname_.has_value()) {
        return fldname_.value();
    }
    return "";
}
