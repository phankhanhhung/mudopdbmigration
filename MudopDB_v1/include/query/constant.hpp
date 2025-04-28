#ifndef CONSTANT_HPP
#define CONSTANT_HPP

#include <optional>
#include <string>
#include <variant>

class Constant {
private:
    std::variant<int, std::string> value_;

public:
    // Constructors
    static Constant with_int(int ival);
    static Constant with_string(const std::string& sval);

    // Getters
    std::optional<int> as_int() const;
    std::optional<std::string> as_string() const;

    // Comparison operators for sorting and equality
    bool operator==(const Constant& other) const;
    bool operator!=(const Constant& other) const;
    bool operator<(const Constant& other) const;
    bool operator<=(const Constant& other) const;
    bool operator>(const Constant& other) const;
    bool operator>=(const Constant& other) const;

    // Hash support
    size_t hash() const;

    // String representation
    std::string to_string() const;

private:
    // Private constructor - use static factory methods
    explicit Constant(std::variant<int, std::string> val);
};

// Hash functor for use in unordered containers
namespace std {
    template <>
    struct hash<Constant> {
        size_t operator()(const Constant& c) const {
            return c.hash();
        }
    };
}

#endif // CONSTANT_HPP
