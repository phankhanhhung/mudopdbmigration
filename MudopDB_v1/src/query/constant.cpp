#include "query/constant.hpp"
#include <functional>
#include <sstream>

// Private constructor
Constant::Constant(std::variant<int, std::string> val) : value_(std::move(val)) {}

// Static factory methods
Constant Constant::with_int(int ival) {
    return Constant(ival);
}

Constant Constant::with_string(const std::string& sval) {
    return Constant(sval);
}

// Getters
std::optional<int> Constant::as_int() const {
    if (std::holds_alternative<int>(value_)) {
        return std::get<int>(value_);
    }
    return std::nullopt;
}

std::optional<std::string> Constant::as_string() const {
    if (std::holds_alternative<std::string>(value_)) {
        return std::get<std::string>(value_);
    }
    return std::nullopt;
}

// Comparison operators
bool Constant::operator==(const Constant& other) const {
    return value_ == other.value_;
}

bool Constant::operator!=(const Constant& other) const {
    return value_ != other.value_;
}

bool Constant::operator<(const Constant& other) const {
    // Handle same-type comparisons
    if (value_.index() == other.value_.index()) {
        return value_ < other.value_;
    }
    // int < string (by convention)
    return value_.index() < other.value_.index();
}

bool Constant::operator<=(const Constant& other) const {
    return *this < other || *this == other;
}

bool Constant::operator>(const Constant& other) const {
    return !(*this <= other);
}

bool Constant::operator>=(const Constant& other) const {
    return !(*this < other);
}

// Hash support
size_t Constant::hash() const {
    return std::visit([](const auto& val) -> size_t {
        return std::hash<std::decay_t<decltype(val)>>{}(val);
    }, value_);
}

// String representation
std::string Constant::to_string() const {
    return std::visit([](const auto& val) -> std::string {
        if constexpr (std::is_same_v<std::decay_t<decltype(val)>, int>) {
            return std::to_string(val);
        } else {
            return val;
        }
    }, value_);
}
