#include "input_options.h"

#include <iostream>
#include <vector>

namespace {
    unsigned checks{};

    InputTest::Options Parse(std::initializer_list<std::wstring_view> arguments) {
        const std::vector<std::wstring_view> values(arguments);
        return InputTest::ParseOptions(values);
    }

    void Check(bool condition) {
        ++checks;
        if (!condition) throw std::runtime_error("Production input option regression failed");
    }

    void Reject(std::initializer_list<std::wstring_view> arguments) {
        ++checks;
        try { Parse(arguments); }
        catch (const std::invalid_argument&) { return; }
        throw std::runtime_error("Invalid production input arguments were accepted");
    }
}

int main() {
    try {
        for (const auto test_case : {L"queue", L"wait-input", L"native-keyboard", L"focus", L"cursor-resize", L"benchmark", L"wait-close"}) {
            const auto options = Parse({L"--case", test_case});
            Check(options.output_directory.empty());
        }
        const auto manual = Parse({L"--case", L"hardware-manual", L"--output-directory", L"F:\\test evidence\\\u9a8c\u8bc1"});
        Check(manual.output_directory.native() == L"F:\\test evidence\\\u9a8c\u8bc1");
        const auto reordered = Parse({L"--output-directory", L"F:\\evidence", L"--case", L"hardware-manual"});
        Check(reordered.test_case == "hardware-manual" && reordered.output_directory.native() == L"F:\\evidence");
        Reject({});
        Reject({L"--case"});
        Reject({L"--case", L""});
        Reject({L"--case", L"--unknown"});
        Reject({L"--case", L"unknown"});
        Reject({L"--case", L"\u9a8c\u8bc1"});
        Reject({L"--case", L"queue", L"--case", L"queue"});
        Reject({L"--case", L"queue", L"--output-directory", L"F:\\evidence"});
        Reject({L"--case", L"hardware-manual"});
        Reject({L"--case", L"hardware-manual", L"--output-directory"});
        Reject({L"--case", L"hardware-manual", L"--output-directory", L""});
        Reject({L"--case", L"hardware-manual", L"--output-directory", L"F:\\evidence", L"--output-directory", L"F:\\other"});
        Reject({L"--case", L"queue", L"--unknown"});
        std::cout << "Production input option checks passed: " << checks << "; CPU-only, no window or GPU.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
