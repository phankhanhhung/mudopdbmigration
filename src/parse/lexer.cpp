/**
 * Lexer implementation.
 * Tokenizes SQL input into keywords, identifiers, numbers, and delimiters.
 */

#include "parse/lexer.hpp"
#include <algorithm>
#include <stdexcept>

namespace parse {

Lexer::Lexer(const std::string& s)
    : keywords_({"select", "from", "where", "and", "insert", "into", "values",
                 "delete", "update", "set", "create", "table", "int", "varchar",
                 "view", "as", "index", "on"}),
      i_(0) {
    for (char c : s) {
        chars_.push_back(c);
    }
    next_token();
}

bool Lexer::match_delim(char d) const {
    if (!token_.has_value()) return false;
    if (token_->ttype != TokenType::Delim) return false;
    if (token_->sval.has_value()) {
        return token_->sval.value()[0] == d;
    }
    return false;
}

bool Lexer::match_int_constant() const {
    if (!token_.has_value()) return false;
    return token_->ttype == TokenType::IntConstant;
}

bool Lexer::match_string_constant() const {
    if (!token_.has_value()) return false;
    return token_->ttype == TokenType::StringConstant;
}

bool Lexer::match_keyword(const std::string& w) const {
    if (!token_.has_value()) return false;
    if (token_->ttype != TokenType::Keyword) return false;
    if (token_->sval.has_value()) {
        return token_->sval.value() == w;
    }
    return false;
}

bool Lexer::match_id() const {
    if (!token_.has_value()) return false;
    return token_->ttype == TokenType::Id;
}

void Lexer::eat_delim(char d) {
    if (!match_delim(d)) {
        throw std::runtime_error("Bad syntax: expected delimiter '" + std::string(1, d) + "'");
    }
    next_token();
}

int32_t Lexer::eat_int_constant() {
    if (!match_int_constant()) {
        throw std::runtime_error("Bad syntax: expected integer constant");
    }
    int32_t val = token_->nval.value();
    next_token();
    return val;
}

std::string Lexer::eat_string_constant() {
    if (!match_string_constant()) {
        throw std::runtime_error("Bad syntax: expected string constant");
    }
    std::string val = token_->sval.value();
    next_token();
    return val;
}

void Lexer::eat_keyword(const std::string& w) {
    if (!match_keyword(w)) {
        throw std::runtime_error("Bad syntax: expected keyword '" + w + "'");
    }
    next_token();
}

std::string Lexer::eat_id() {
    if (!match_id()) {
        throw std::runtime_error("Bad syntax: expected identifier");
    }
    std::string val = token_->sval.value();
    next_token();
    return val;
}

void Lexer::next_token() {
    if (i_ >= chars_.size()) {
        token_ = std::nullopt;
        return;
    }

    // Skip whitespace
    while (is_whitespace_char(chars_[i_])) {
        i_++;
        if (i_ >= chars_.size()) {
            token_ = std::nullopt;
            return;
        }
    }

    char ch = chars_[i_];

    // String constants
    if (ch == '"' || ch == '\'') {
        i_++;
        if (i_ >= chars_.size()) {
            token_ = std::nullopt;
            return;
        }
        std::string sval;
        char c = chars_[i_];
        while (c != ch) {
            sval.push_back(c);
            i_++;
            if (i_ >= chars_.size()) {
                token_ = std::nullopt;
                return;
            }
            c = chars_[i_];
        }
        Token tok;
        tok.sval = sval;
        tok.ttype = TokenType::StringConstant;
        token_ = tok;
        i_++;
        return;
    }

    // Negative numbers
    bool is_negative = false;
    if (chars_[i_] == '-') {
        is_negative = true;
        i_++;
    }

    // Numbers
    int32_t nval = 0;
    bool is_number_found = false;
    while (i_ < chars_.size() && is_number(chars_[i_])) {
        is_number_found = true;
        nval = 10 * nval + (chars_[i_] - '0');
        i_++;
    }
    if (is_number_found) {
        if (is_negative) nval = -nval;
        Token tok;
        tok.nval = nval;
        tok.ttype = TokenType::IntConstant;
        token_ = tok;
        return;
    }
    if (is_negative) {
        i_--;
    }

    // Words (identifiers/keywords)
    std::string sval;
    bool is_first = true;
    while (i_ < chars_.size() && is_word_char(chars_[i_], is_first)) {
        sval.push_back(chars_[i_]);
        i_++;
        is_first = false;
    }
    if (!sval.empty()) {
        // Convert to lowercase
        std::transform(sval.begin(), sval.end(), sval.begin(), ::tolower);
        Token tok;
        tok.sval = sval;
        if (keywords_.count(sval)) {
            tok.ttype = TokenType::Keyword;
        } else {
            tok.ttype = TokenType::Id;
        }
        token_ = tok;
        return;
    }

    // Delimiters
    if (i_ < chars_.size() && is_delim_char(chars_[i_])) {
        Token tok;
        tok.sval = std::string(1, chars_[i_]);
        tok.ttype = TokenType::Delim;
        token_ = tok;
        i_++;
        return;
    }

    token_ = std::nullopt;
}

bool Lexer::is_whitespace_char(char c) const {
    return c >= '\0' && c <= ' ';
}

bool Lexer::is_number(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::is_word_char(char c, bool is_first) const {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return true;
    if (c >= '\xA0' && c <= '\xFF') return true;
    if (c == '_') return true;
    if (!is_first && is_number(c)) return true;
    return false;
}

bool Lexer::is_delim_char(char c) const {
    return !is_whitespace_char(c) && !is_number(c) && !is_word_char(c, true);
}

} // namespace parse
