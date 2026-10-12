#include "benchmark/runner.h"
#include <iostream>

int wmain(int argc, wchar_t** argv)
{
    if (argc != 3) return 64;
    try {
        const auto capture = Benchmark::fs::path(argv[1]);
        const auto meta = Benchmark::ReadYaml(capture / "summary.yaml")["metadata"];
        const Benchmark::Round round{meta["benchmark"].as<std::string>(), 1,
            meta["warmup_seconds"].as<unsigned>(), meta["sample_seconds"].as<unsigned>()};
        Benchmark::ValidateCapture(capture, round, meta["focus_policy"].as<std::string>(), 0,
            Benchmark::Utf8(argv[2]));
        std::cout << "Native benchmark consumer accepted " << Benchmark::Utf8(argv[2]) << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
