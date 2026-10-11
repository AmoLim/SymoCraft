#pragma once

#include <filesystem>
#include <initializer_list>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace InputTest {
    struct Options {
        std::string test_case;
        std::filesystem::path output_directory;
    };

    inline Options ParseOptions(std::span<const std::wstring_view> arguments) {
        Options result;
        bool case_seen{}, output_seen{};
        for (std::size_t index = 0; index < arguments.size(); ++index) {
            const auto argument = arguments[index];
            if (argument != L"--case" && argument != L"--output-directory")
                throw std::invalid_argument("Unknown production input option");
            if (++index == arguments.size() || arguments[index].empty() || arguments[index].starts_with(L"--"))
                throw std::invalid_argument("Missing production input option value");
            if (argument == L"--case") {
                if (case_seen) throw std::invalid_argument("Duplicate --case");
                case_seen = true;
                for (wchar_t character : arguments[index]) {
                    if (character > 127) throw std::invalid_argument("Test case names must use ASCII");
                    result.test_case.push_back(static_cast<char>(character));
                }
            } else {
                if (output_seen) throw std::invalid_argument("Duplicate --output-directory");
                output_seen = true;
                result.output_directory = std::filesystem::path(arguments[index]);
            }
        }
        bool known_case{};
        for (const char* known : {"queue", "wait-input", "native-keyboard", "focus", "cursor-resize", "benchmark", "wait-close", "hardware-manual"})
            known_case = known_case || result.test_case == known;
        if (!case_seen || !known_case) throw std::invalid_argument("Unknown or missing production input test case");
        if (result.test_case == "hardware-manual") {
            if (!output_seen) throw std::invalid_argument("hardware-manual requires --output-directory FRESH_EXISTING_DIRECTORY");
        } else if (output_seen) throw std::invalid_argument("Only hardware-manual accepts --output-directory");
        return result;
    }
}
