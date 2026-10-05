#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <thread>

int main(int argc, char** argv)
{
    std::string_view seed;
    std::filesystem::path output;
    for (int i = 1; i + 1 < argc; ++i) {
        const std::string_view argument = argv[i];
        if (argument == "--seed") seed = argv[++i];
        else if (argument == "--output") output = argv[++i];
    }
    // A headless subprocess fixture, not a real game or performance measurement.
    if (seed != "2") std::this_thread::sleep_for(std::chrono::milliseconds(150));
    if (seed == "3") {
        std::cerr << "[runtime] fatal: fixture failure\n";
        std::cout << "[runtime] shutdown complete; exit_code=3\n";
        return 3;
    }
    if (output.empty() || !std::filesystem::create_directory(output)) return 64;
    const bool focused = seed != "4";
    std::ofstream frames(output / "frames.csv");
    frames << "phase,frame_ms,gpu_draw_ms,upload_bytes,upload_cpu_ms,mesh_ms,present_ms,edits,focused,x,y\n"
           << "warmup,20,,100,1,0,1,0,1,1,2\n"
           << "sample,10,0.5,100,1,0,1,0,1,1,2\n"
           << "sample,30,,100,1,0,1,0," << focused << ",1,2\n";
    std::ofstream summary(output / "summary.yaml");
    summary << "completed: true\nvalid_run: " << (focused ? "true" : "false") << '\n';
    const int exit_code = focused ? 0 : 4;
    std::cout << "[performance] exported; valid_run=" << focused << '\n'
              << "[runtime] shutdown complete; exit_code=" << exit_code << '\n';
    return exit_code;
}
