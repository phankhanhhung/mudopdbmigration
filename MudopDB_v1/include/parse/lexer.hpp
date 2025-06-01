#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>
#include <unordered_set>
#include <optional>

namespace parse {

/**
 * Lexer tokenizes SQL input strings.
 *
 * Corresponds to Lexer in Rust (NMDB2/src/parse/lexer.rs)
 */
class Lexer {
public:
    explicit Lexer(const std::string& s);

    bool match_delim(char d) const;
    bool match_int_constant() const;
    bool match_string_constant() const;
    bool match_keyword(const std::string& w) const;
    bool match_id() const;

    void eat_delim(char d);
    int32_t eat_int_constant();
    std::string eat_string_constant();
    void eat_keyword(const std::string& w);
    std::string eat_id();

private:
    enum class TokenType { Delim, IntConstant, StringConstant, Keyword, Id };

    struct Token {
        std::optional<int32_t> nval;
        std::optional<std::string> sval;
        TokenType ttype;
    };

    void next_token();
    bool is_whitespace_char(char c) const;
    bool is_number(char c) const;
    bool is_word_char(char c, bool is_first) const;
    bool is_delim_char(char c) const;

    std::unordered_set<std::string> keywords_;
    std::vector<char> chars_;
    size_t i_;
    std::optional<Token> token_;
};

} // namespace parse

#endif // LEXER_HPP
