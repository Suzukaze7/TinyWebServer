#include "include/json.h"
#include "include/exception.h"
#include <charconv>
#include <exception>
#include <format>
#include <iostream>
#include <iterator>
#include <memory>
#include <string_view>
#include <system_error>
#include <utility>

namespace suzukaze {
namespace json {
auto Value::operator[](std::size_t idx) -> Value & {
    try {
        return get<Array>().at(idx);
    } catch (std::out_of_range &e) {
        throw JsonException(e.what());
    }
}

auto Value::operator[](const String &key) -> Value & {
    try {
        return get<Object>().at(key);
    } catch (std::out_of_range &e) {
        throw JsonException(e.what());
    }
}

auto Parser::create_token(TokenType type, std::size_t len) -> Token {
    auto value = json_.substr(0, len);
    json_.remove_prefix(len);
    return {type, value};
}

auto Parser::next_token() -> Token {
    json_.remove_prefix(std::ranges::find_if_not(json_, isspace) - json_.begin());

    if (json_.empty())
        return {TokenType::END};

    char c = json_.front();
    switch (c) {
    case '{':
        return create_token(TokenType::LEFT_BRACE);
    case '}':
        return create_token(TokenType::RIGHT_BRACE);
    case '[':
        return create_token(TokenType::LEFT_BRACKET);
    case ']':
        return create_token(TokenType::RIGHT_BRACKET);
    case ',':
        return create_token(TokenType::COMMA);
    case ':':
        return create_token(TokenType::COLON);
    }

    if (c == '"') {
        auto pos = json_.find('"', 1);
        if (pos == view::npos)
            throw JsonException("parse: unterminated string");

        return create_token(TokenType::STRING, pos + 1);
    }

    if (std::isdigit(c) || c == '-') {
        auto it = std::ranges::find_if_not(json_.begin(), json_.end(), isdigit);
        if (it == json_.end() || *it != '.')
            return create_token(TokenType::INT, it - json_.begin());

        it = std::ranges::find_if_not(it + 1, json_.end(), isdigit);
        return create_token(TokenType::FLOAT, it - json_.begin());
    }

    if (json_.starts_with("true"))
        return create_token(TokenType::BOOL, 4);

    if (json_.starts_with("false"))
        return create_token(TokenType::BOOL, 5);

    if (json_.starts_with("null"))
        return create_token(TokenType::NUL, 4);

    throw JsonException("parse: invalid character");
}

void Parser::consume(TokenType expected) {
    if (cur_token_.type_ != expected)
        throw JsonException("parse: syntax error");

    cur_token_ = next_token();
}

auto Parser::parse_object() -> Value {
    consume(TokenType::LEFT_BRACE);
    Object obj;
    while (true) {
        auto value = cur_token_.val_;
        consume(TokenType::STRING);
        consume(TokenType::COLON);
        obj.insert_or_assign(String(std::next(value.begin()), std::prev(value.end())),
                             parse_value());
        switch (cur_token_.type_) {
        case TokenType::COMMA:
            consume(TokenType::COMMA);
            break;
        case TokenType::RIGHT_BRACE:
            consume(TokenType::RIGHT_BRACE);
            return obj;
        default:
            throw JsonException("parse: syntax error");
        }
    }
}

auto Parser::parse_array() -> Value {
    consume(TokenType::LEFT_BRACKET);
    Array arr;
    while (true) {
        arr.push_back(parse_value());
        switch (cur_token_.type_) {
        case TokenType::COMMA:
            consume(TokenType::COMMA);
            break;
        case TokenType::RIGHT_BRACKET:
            consume(TokenType::RIGHT_BRACKET);
            return arr;
        default:
            throw JsonException("parse: syntax error");
        }
    }
}

auto Parser::parse_string() -> Value {
    auto &v = cur_token_.val_;
    String s(std::next(v.begin()), std::prev(v.end()));
    consume(TokenType::STRING);
    return std::move(s);
}

auto Parser::parse_int() -> Value {
    auto &v = cur_token_.val_;
    Int resi;
    std::from_chars(v.data(), v.data() + v.size(), resi);
    consume(TokenType::INT);
    return resi;
}

auto Parser::parse_float() -> Value {
    auto &v = cur_token_.val_;
    Float resf;
    std::from_chars(v.data(), v.data() + v.size(), resf);
    consume(TokenType::FLOAT);
    return resf;
}

auto Parser::parse_bool() -> Value {
    Bool b = cur_token_.val_.size() == 4;
    consume(TokenType::BOOL);
    return b;
}

auto Parser::parse_value() -> Value {
    switch (cur_token_.type_) {
    case TokenType::LEFT_BRACE:
        return parse_object();
    case TokenType::LEFT_BRACKET:
        return parse_array();
    case TokenType::INT:
        return parse_int();
    case TokenType::FLOAT:
        return parse_float();
    case TokenType::STRING:
        return parse_string();
    case TokenType::BOOL:
        return parse_bool();
    case TokenType::NUL:
        consume(TokenType::NUL);
        return {};
    default:
        throw JsonException("syntax error");
    }
}

auto Parser::parse(view json) -> Value {
    json_ = json;
    cur_token_ = next_token();
    return parse_value();
}

void Serializer::serialize_value(const Value &val) {
    if (auto ptr = val.get_if<Object>())
        serialize_object(*ptr);
    else if (auto ptr = val.get_if<Array>())
        serialize_array(*ptr);
    else if (auto ptr = val.get_if<String>())
        serialize_string(*ptr);
    else if (auto ptr = val.get_if<Int>())
        std::format_to(it_, "{}", *ptr);
    else if (auto ptr = val.get_if<Float>())
        std::format_to(it_, "{}", *ptr);
    else if (auto ptr = val.get_if<Bool>())
        std::format_to(it_, "{}", *ptr);
    else if (auto ptr = val.get_if<Null>())
        std::format_to(it_, "null");
    else
        throw JsonException("serialize: error json type");
}

void Serializer::serialize_object(const Object &obj) {
    json_ += '{';
    for (auto &[k, v] : obj) {
        serialize_string(k);
        json_ += ':';
        serialize_value(v);
        json_ += ',';
    }
    json_.back() = '}';
}

void Serializer::serialize_array(const Array &arr) {
    json_ += '[';
    for (auto &v : arr) {
        serialize_value(v);
        json_ += ',';
    }
    json_.back() = ']';
}

void Serializer::serialize_string(const String &str) { std::format_to(it_, "\"{}\"", str); }

std::string Serializer::serialize(const Value &val) {
    json_.clear();
    serialize_value(val);
    return std::move(json_);
}

} // namespace json
} // namespace suzukaze