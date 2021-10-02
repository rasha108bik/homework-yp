#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>

using namespace std;

namespace parse {

    bool operator==(const Token& lhs, const Token& rhs) {
        using namespace token_type;

        if (lhs.index() != rhs.index()) {
            return false;
        }
        if (lhs.Is<Char>()) {
            return lhs.As<Char>().value == rhs.As<Char>().value;
        }
        if (lhs.Is<Number>()) {
            return lhs.As<Number>().value == rhs.As<Number>().value;
        }
        if (lhs.Is<String>()) {
            return lhs.As<String>().value == rhs.As<String>().value;
        }
        if (lhs.Is<Id>()) {
            return lhs.As<Id>().value == rhs.As<Id>().value;
        }
        return true;
    }

    bool operator!=(const Token& lhs, const Token& rhs) {
        return !(lhs == rhs);
    }

    std::ostream& operator<<(std::ostream& os, const Token& rhs) {
        using namespace token_type;

#define VALUED_OUTPUT(type) \
if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

        VALUED_OUTPUT(Number);
        VALUED_OUTPUT(Id);
        VALUED_OUTPUT(String);
        VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
if (rhs.Is<type>()) return os << #type;

        UNVALUED_OUTPUT(Class);
        UNVALUED_OUTPUT(Return);
        UNVALUED_OUTPUT(If);
        UNVALUED_OUTPUT(Else);
        UNVALUED_OUTPUT(Def);
        UNVALUED_OUTPUT(Newline);
        UNVALUED_OUTPUT(Print);
        UNVALUED_OUTPUT(Indent);
        UNVALUED_OUTPUT(Dedent);
        UNVALUED_OUTPUT(And);
        UNVALUED_OUTPUT(Or);
        UNVALUED_OUTPUT(Not);
        UNVALUED_OUTPUT(Eq);
        UNVALUED_OUTPUT(NotEq);
        UNVALUED_OUTPUT(LessOrEq);
        UNVALUED_OUTPUT(GreaterOrEq);
        UNVALUED_OUTPUT(None);
        UNVALUED_OUTPUT(True);
        UNVALUED_OUTPUT(False);
        UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

        return os << "Unknown token :("sv;
    }

    Lexer::Lexer(std::istream& input)
    : input_(input) {
        parseLine();
        token_ = ParseLexer();
    }

    const Token& Lexer::CurrentToken() const {
        return token_;
    }

    Token Lexer::NextToken() {
        token_ = ParseLexer();
        return token_;
    }

    bool Lexer::parseLine() {
        line_ = ""s;
        current_pos_ = 0;
        sign_found_ = false;
        while (!CheckLineHasSign(line_)) {
            if (!std::getline(input_, line_)) {
                return false;
            }
        }
        return true;
    }

    bool Lexer::CheckLineHasSign(string_view line) {
        for (size_t i = 0; i < line.size(); i++) {
            if (line[i] == '#')
                return false;
            if (line[i] != ' ')
                return true;
        }
        return false;
    }

    Token Lexer::ParseLexer() {
        static size_t space_cnt = 0;

        if (line_.empty()) {
            if (parseLine()) {
                return Token{token_type::Newline()};
            }
            else if (space_cnt > 0) {
                space_cnt -= 2;
                return Token{token_type::Dedent()};
            }
            return Token{token_type::Eof()};
        }
        if (current_pos_ >= line_.size()) {
            parseLine();
            return Token{token_type::Newline()};
        }
        size_t spaces = (!sign_found_) ? current_pos_ : 0;
        for (size_t i = current_pos_; i < line_.size(); ++i) {
            ++current_pos_;
            const char sign = line_[i];
            if (sign == ' ' && !sign_found_) {
                ++spaces;
                if (spaces == space_cnt + 2 && CheckLineHasSign(line_)) {
                    space_cnt += 2;
                    return Token{token_type::Indent()};
                }
            }
            else {
                if (!sign_found_ && (space_cnt - spaces) >= 2 && (space_cnt - spaces) % 2 == 0) {
                    space_cnt -= 2;
                    --current_pos_;
                    return Token{token_type::Dedent()};
                }
                sign_found_ = true;
                if (sign == ' ') {
                    continue;
                }
                else if (sign == '\"' || sign == '\'') {
                    return ParseString(line_.substr(i));
                }
                else if (std::isdigit(sign)) {
                    return ParseNumber(line_.substr(i));
                }

                if (sign == '=' || sign == '<' || sign == '>' || sign == '!') {
                    if (i != line_.size() - 1 && line_[i + 1] == '=') {
                        return ParseSign(sign);
                    }
                }
                else if (std::isalpha(sign) || sign == '_') {
                    return ParseIdentifier(line_.substr(i));
                }
                else if (sign == '#') {
                    if (parseLine()) {
                        return Token{token_type::Newline()};
                    }
                    return Token{token_type::Eof()};
                }

                if (std::iscntrl(sign)) {
                    if (current_pos_ >= line_.size()) {
                        parseLine();
                    }
                    return Token{token_type::Newline()};
                }
                else {
                    token_type::Char cha{};
                    cha.value = sign;
                    return Token{cha};
                }
            }
        }
        return Token{token_type::Eof()};
    }

    Token Lexer::ParseString(std::string_view str) {
        char type_sign = str[0];
        str.remove_prefix(1);
        auto it = str.begin();
        auto end = str.end();
        std::string str_result;
        bool si_found = false;
        while (it != end) {
            const char current_char = *it;
            if (current_char == type_sign) {
                ++it;
                si_found = true;
                break;
            }
            else if (current_char == '\\') {
                ++it;
                const char escaped_char = *(it);
                switch (escaped_char) {
                    case 'n':
                        str_result.push_back('\n');
                        break;
                        case 't':
                            str_result.push_back('\t');
                            break;
                            case 'r':
                                str_result.push_back('\r');
                                break;
                                case '\"':
                                    str_result.push_back('\"');
                                    break;
                                    case '\'':
                                        str_result.push_back('\'');
                                        break;
                                        case '\\':
                                            str_result.push_back('\\');
                                            break;
                                            default:
                                                throw LexerError("Not implemented"s);
                }
            }
            else {
                str_result.push_back(current_char);
            }
            ++it;
        }
        if (si_found) {
            token_type::String word;
            word.value = str_result;
            current_pos_ += (std::distance(str.begin(), it));
            return Token(word);
        }
        token_type::Char cha;
        cha.value = type_sign;
        return Token(cha);
    }

    Token Lexer::ParseNumber(std::string_view str) {
        token_type::Number num{};
        for (size_t i = 0; i < str.size(); ++i) {
            if (!std::isdigit(str[i])) {
                num.value = stoi(std::string(str.substr(0, i)));
                current_pos_ += (i - 1);
                return Token{num};
            }
        }
        num.value = stoi(std::string(str.substr(0)));
        current_pos_ += (str.size() - 1);
        return Token{num};
    }

    Token Lexer::ParseIdentifier(std::string_view str) {
        std::string_view id_str;
        for (size_t i = 0; i < str.size(); ++i) {
            if (!std::isalnum(str[i]) && str[i] != '_') {
                id_str = str.substr(0, i);
                current_pos_ += (i - 1);
                break;
            }
        }
        if (id_str.empty()) {
            id_str = str.substr(0);
            current_pos_ += str.size();
        }
        auto name = std::string(id_str);
        if (tokens_.count(name) > 0) {
            return tokens_.at(name);
        }
        token_type::Id id;
        id.value = name;
        return Token(id);
    }

    Token Lexer::ParseSign(char ch) {
        current_pos_++;
        if (ch == '=') {
            return Token(token_type::Eq());
        }
        if (ch == '<') {
            return Token(token_type::LessOrEq());
        }
        if (ch == '!') {
            return Token(token_type::NotEq());
        }
        else {
            return Token(token_type::GreaterOrEq());
        }
    }

}  // namespace parse