#include "symocraft/foundation/document.h"

namespace SymoCraft::Data {
    Value& Value::operator[](std::string_view key) {
        if (!IsDefined()) value_ = Mapping{};
        if (!IsMap()) throw std::invalid_argument("Document value is not a mapping");
        auto& mapping = std::get<Mapping>(value_);
        for (auto& [name, value] : mapping) if (name == key) return value;
        mapping.emplace_back(std::string(key), Value{});
        return mapping.back().second;
    }
    const Value& Value::operator[](std::string_view key) const {
        if (IsMap()) for (const auto& [name, value] : std::get<Mapping>(value_))
            if (name == key) return value;
        static const Value missing;
        return missing;
    }
    void Value::push_back(Value value) {
        if (!IsDefined()) value_ = Sequence{};
        if (!IsSequence()) throw std::invalid_argument("Document value is not a sequence");
        std::get<Sequence>(value_).push_back(std::move(value));
    }
    std::size_t Value::size() const {
        if (IsMap()) return std::get<Mapping>(value_).size();
        if (IsSequence()) return std::get<Sequence>(value_).size();
        return 0;
    }
}
