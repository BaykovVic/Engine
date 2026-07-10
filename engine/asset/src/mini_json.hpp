#pragma once

// Minimal JSON parser for the engine's own importers (glTF). Internal to
// the asset module — not part of the public contract surface.

#include <cctype>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sky::asset::detail {

struct JsonValue {
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool boolean = false;
    double number = 0.0;
    std::string string;
    std::vector<JsonValue> array;
    std::vector<std::pair<std::string, JsonValue>> object;

    [[nodiscard]] const JsonValue* find(std::string_view key) const {
        if (type != Type::Object) {
            return nullptr;
        }
        for (const auto& [name, value] : object) {
            if (name == key) {
                return &value;
            }
        }
        return nullptr;
    }

    [[nodiscard]] double numberOr(double fallback) const {
        return type == Type::Number ? number : fallback;
    }

    [[nodiscard]] const JsonValue* at(std::size_t index) const {
        return type == Type::Array && index < array.size() ? &array[index] : nullptr;
    }
};

class JsonParser {
public:
    explicit JsonParser(std::string_view text) : text_(text) {}

    std::optional<JsonValue> parse() {
        auto value = parseValue();
        skipWhitespace();
        if (!value || position_ != text_.size()) {
            return std::nullopt;
        }
        return value;
    }

private:
    void skipWhitespace() {
        while (position_ < text_.size() &&
               std::isspace(static_cast<unsigned char>(text_[position_]))) {
            ++position_;
        }
    }

    bool consume(char expected) {
        skipWhitespace();
        if (position_ < text_.size() && text_[position_] == expected) {
            ++position_;
            return true;
        }
        return false;
    }

    std::optional<JsonValue> parseValue() {
        skipWhitespace();
        if (position_ >= text_.size()) {
            return std::nullopt;
        }
        switch (text_[position_]) {
            case '{': return parseObject();
            case '[': return parseArray();
            case '"': return parseString();
            case 't':
            case 'f': return parseBool();
            case 'n': return parseNull();
            default: return parseNumber();
        }
    }

    std::optional<JsonValue> parseObject() {
        JsonValue value;
        value.type = JsonValue::Type::Object;
        ++position_; // '{'
        skipWhitespace();
        if (consume('}')) {
            return value;
        }
        for (;;) {
            auto key = parseString();
            if (!key || !consume(':')) {
                return std::nullopt;
            }
            auto member = parseValue();
            if (!member) {
                return std::nullopt;
            }
            value.object.emplace_back(std::move(key->string), std::move(*member));
            if (consume(',')) {
                continue;
            }
            if (consume('}')) {
                return value;
            }
            return std::nullopt;
        }
    }

    std::optional<JsonValue> parseArray() {
        JsonValue value;
        value.type = JsonValue::Type::Array;
        ++position_; // '['
        skipWhitespace();
        if (consume(']')) {
            return value;
        }
        for (;;) {
            auto element = parseValue();
            if (!element) {
                return std::nullopt;
            }
            value.array.push_back(std::move(*element));
            if (consume(',')) {
                continue;
            }
            if (consume(']')) {
                return value;
            }
            return std::nullopt;
        }
    }

    std::optional<JsonValue> parseString() {
        skipWhitespace();
        if (position_ >= text_.size() || text_[position_] != '"') {
            return std::nullopt;
        }
        ++position_;
        JsonValue value;
        value.type = JsonValue::Type::String;
        while (position_ < text_.size() && text_[position_] != '"') {
            char c = text_[position_++];
            if (c == '\\' && position_ < text_.size()) {
                const char escaped = text_[position_++];
                switch (escaped) {
                    case 'n': c = '\n'; break;
                    case 't': c = '\t'; break;
                    case 'r': c = '\r'; break;
                    case 'b': c = '\b'; break;
                    case 'f': c = '\f'; break;
                    case 'u':
                        // Importers only need ASCII names; skip the escape.
                        position_ += 4;
                        c = '?';
                        break;
                    default: c = escaped; break;
                }
            }
            value.string.push_back(c);
        }
        if (position_ >= text_.size()) {
            return std::nullopt;
        }
        ++position_; // closing quote
        return value;
    }

    std::optional<JsonValue> parseBool() {
        JsonValue value;
        value.type = JsonValue::Type::Bool;
        if (text_.substr(position_, 4) == "true") {
            value.boolean = true;
            position_ += 4;
            return value;
        }
        if (text_.substr(position_, 5) == "false") {
            position_ += 5;
            return value;
        }
        return std::nullopt;
    }

    std::optional<JsonValue> parseNull() {
        if (text_.substr(position_, 4) == "null") {
            position_ += 4;
            return JsonValue{};
        }
        return std::nullopt;
    }

    std::optional<JsonValue> parseNumber() {
        const auto start = position_;
        while (position_ < text_.size() &&
               (std::isdigit(static_cast<unsigned char>(text_[position_])) ||
                text_[position_] == '-' || text_[position_] == '+' ||
                text_[position_] == '.' || text_[position_] == 'e' ||
                text_[position_] == 'E')) {
            ++position_;
        }
        if (start == position_) {
            return std::nullopt;
        }
        JsonValue value;
        value.type = JsonValue::Type::Number;
        value.number = std::strtod(std::string(text_.substr(start, position_ - start)).c_str(),
                                   nullptr);
        return value;
    }

    std::string_view text_;
    std::size_t position_ = 0;
};

inline std::optional<JsonValue> parseJson(std::string_view text) {
    return JsonParser(text).parse();
}

} // namespace sky::asset::detail
