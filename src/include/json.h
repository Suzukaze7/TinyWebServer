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

    Value &operator[](std::size_t idx);
    Value &operator[](const String &key);

    template <typename T>
    T &get() {
        if (auto ptr = std::get_if<T>(this))
            return *ptr;
        throw JsonException("type error");
    }

    template <typename T>
    const T &get() const {
        if (auto ptr = std::get_if<T>(this))
            return *ptr;
        throw JsonException("type error");
    }

    template <typename T>
    T *get_if() {
        return std::get_if<T>(this);
    }

    template <typename T>
    const T *get_if() const {
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

    Token create_token(TokenType type, std::size_t len = 1);
    Token next_token();

    void consume(TokenType expected);
    Value parse_value();
    Value parse_object();
    Value parse_array();
    Value parse_string();
    Value parse_int();
    Value parse_float();
    Value parse_bool();

public:
    Value parse(view json);
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