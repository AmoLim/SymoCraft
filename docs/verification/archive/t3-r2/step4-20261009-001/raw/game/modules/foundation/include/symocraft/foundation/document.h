#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace SymoCraft::Data {
    // Owned protocol values: copies never alias a parser node or another document.
    class Value {
    public:
        using Sequence = std::vector<Value>;
        using Mapping = std::vector<std::pair<std::string, Value>>;
        using Storage = std::variant<std::monostate, bool, std::int64_t, std::uint64_t,
                                     float, double, std::string, Sequence, Mapping>;
        Value() = default;
        Value(bool value) : value_(value) {}
        Value(const char* value) : value_(std::string(value)) {}
        Value(std::string value) : value_(std::move(value)) {}
        Value(std::string_view value) : value_(std::string(value)) {}
        Value(Sequence value) : value_(std::move(value)) {}
        Value(Mapping value) : value_(std::move(value)) {}
        template<class T> requires (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
        Value(T value) {
            if constexpr (std::is_same_v<T, float>) value_ = value;
            else if constexpr (std::is_floating_point_v<T>) value_ = static_cast<double>(value);
            else if constexpr (std::is_signed_v<T>) value_ = static_cast<std::int64_t>(value);
            else value_ = static_cast<std::uint64_t>(value);
        }
        template<class T> Value(const std::vector<T>& values) : value_(Sequence{}) {
            for (const auto& value : values) push_back(Value(value));
        }
        Value& operator[](std::string_view key);
        const Value& operator[](std::string_view key) const;
        Value& operator[](std::size_t index) { return std::get<Sequence>(value_).at(index); }
        const Value& operator[](std::size_t index) const { return std::get<Sequence>(value_).at(index); }
        void push_back(Value value);
        bool IsDefined() const { return !std::holds_alternative<std::monostate>(value_); }
        bool IsMap() const { return std::holds_alternative<Mapping>(value_); }
        bool IsSequence() const { return std::holds_alternative<Sequence>(value_); }
        bool IsScalar() const { return IsDefined() && !IsMap() && !IsSequence(); }
        explicit operator bool() const { return IsDefined(); }
        std::size_t size() const;
        void SetFlowStyle(bool enabled = true) { flow_style_ = enabled; }
        bool FlowStyle() const { return flow_style_; }
        const Storage& storage() const { return value_; }
        template<class T> T as() const {
            return std::visit([](const auto& value) -> T {
                using U = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, U>) return value;
                else if constexpr (std::is_arithmetic_v<T> && std::is_arithmetic_v<U>)
                    return static_cast<T>(value);
                else throw std::invalid_argument("Document value has a different type");
            }, value_);
        }
    private:
        Storage value_;
        bool flow_style_ = false;
    };
}
