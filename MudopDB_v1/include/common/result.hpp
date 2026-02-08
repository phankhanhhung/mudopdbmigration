#ifndef DBRESULT_HPP
#define DBRESULT_HPP

#include <optional>
#include <string>
#include <stdexcept>
#include <utility>

/**
 * DbResult<T> - Explicit error handling for database operations.
 *
 * Mirrors Rust's Result<T, TransactionError> pattern.
 * Success holds a value of type T; failure holds an error message.
 *
 * Usage:
 *   DbResult<int> r = DbResult<int>::ok(42);
 *   DbResult<int> r = DbResult<int>::err("not found");
 *   if (r.is_ok()) { use(r.value()); }
 *   int v = r.value();  // throws std::runtime_error if err
 */
template<typename T>
class DbResult {
public:
    static DbResult ok(T val) {
        DbResult r;
        r.value_.emplace(std::move(val));
        return r;
    }

    static DbResult err(const std::string& msg) {
        DbResult r;
        r.error_.emplace(msg);
        return r;
    }

    bool is_ok() const { return value_.has_value(); }
    bool is_err() const { return error_.has_value(); }
    explicit operator bool() const { return is_ok(); }

    T& value() {
        if (is_err()) throw std::runtime_error(error_.value());
        return *value_;
    }

    const T& value() const {
        if (is_err()) throw std::runtime_error(error_.value());
        return *value_;
    }

    const std::string& error() const {
        static const std::string empty;
        return error_ ? *error_ : empty;
    }

private:
    DbResult() = default;
    std::optional<T> value_;
    std::optional<std::string> error_;
};

/**
 * DbResult<void> specialization for operations that return no value.
 */
template<>
class DbResult<void> {
public:
    static DbResult ok() {
        return DbResult{};
    }

    static DbResult err(const std::string& msg) {
        DbResult r;
        r.error_.emplace(msg);
        return r;
    }

    bool is_ok() const { return !error_.has_value(); }
    bool is_err() const { return error_.has_value(); }
    explicit operator bool() const { return is_ok(); }

    void value() const {
        if (is_err()) throw std::runtime_error(error_.value());
    }

    const std::string& error() const {
        static const std::string empty;
        return error_ ? *error_ : empty;
    }

private:
    DbResult() = default;
    std::optional<std::string> error_;
};

#endif // DBRESULT_HPP
