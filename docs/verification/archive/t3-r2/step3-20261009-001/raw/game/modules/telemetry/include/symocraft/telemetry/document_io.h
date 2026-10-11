#pragma once
#include "symocraft/foundation/document.h"
#include <filesystem>
#include <string>
#include <string_view>

namespace SymoCraft::Data {
    std::string DumpYaml(const Value& value);
    Value LoadYaml(std::string_view text);
    Value LoadYamlFile(const std::filesystem::path& path);
}
