#pragma once
#include "exception.h"
#include <cstdint>
#include <iterator>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace suzukaze {
namespace json {
class Value;

using Null = std::monostate;
using Bool = bool;
using Int = std::int64_t;
using Float = double;
using String = std::string;
using Array = std::vector<Value>;
using Object = std::map<String, Value>;
using Base = std::variant<Null, bool, Int, Float, String, Array, Object>;

constexpr std::monostate null;

class Value : private Base {
public:
    using Base::variant;

    auto operator[](std::size_t idx) -> Value &;
    auto operator[](const String &key) -> Value &;

    template <typename T>
    auto get() -> T & {
        if (auto ptr = std::get_if<T>(this))
            return *ptr;
        throw JsonException("type error");
    }

    template <typename T>
    auto get() const -> const T & {
        if (auto ptr = std::get_if<T>(this))
            return *ptr;
        throw JsonException("type error");
    }

    template <typename T>
    auto get_if() -> T * {
        return std::get_if<T>(this);
    }

    template <typename T>
    auto get_if() const -> const T * {
        return std::get_if<T>(this);
    }
};

class Parser {
    using view = std::string_view;

    enum class TokenType {
        LEFT_BRACE,
        RIGHT_BRACE,
        LEFT_BRACKET,
        RIGHT_BRACKET,
        COMMA,
        COLON,
        STRING,
        INT,
        FLOAT,
        BOOL,
        NUL,
        END
    };

    struct Token {
        TokenType type_;
        view val_;
    };

    view json_;
    Token cur_token_;

    auto create_token(TokenType type, std::size_t len = 1) -> Token;
    auto next_token() -> Token;

    void consume(TokenType expected);
    auto parse_value() -> Value;
    auto parse_object() -> Value;
    auto parse_array() -> Value;
    auto parse_string() -> Value;
    auto parse_int() -> Value;
    auto parse_float() -> Value;
    auto parse_bool() -> Value;

public:
    auto parse(view json) -> Value;
};

class Serializer {
    std::string json_;
    std::back_insert_iterator<std::string> it_{json_};

    void serialize_value(const Value &val);
    void serialize_object(const Object &obj);
    void serialize_array(const Array &arr);
    void serialize_string(const String &str);

public:
    std::string serialize(const Value &val);
};
} // namespace json
} // namespace suzukaze