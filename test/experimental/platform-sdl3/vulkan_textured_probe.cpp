#include "vulkan_probe_common.h"

using namespace SymoCraftVulkanExperiment;

int main(int argc, char** argv) {
    std::cout << std::unitbuf;
    std::filesystem::path vertex, fragment, output;
    auto state = std::make_unique<Context>();
    int result = 0;
    try {
        for (int i = 1; i < argc; i += 2) {
            Require(i + 1 < argc, "CLI requires --vertex --fragment --output pairs");
            const std::string option = argv[i];
            if (option == "--vertex") vertex = argv[i + 1];
            else if (option == "--fragment") fragment = argv[i + 1];
            else if (option == "--output") output = argv[i + 1];
            else throw std::runtime_error("unknown CLI option");
        }
        Require(!vertex.empty() && !fragment.empty() && !output.empty(), "CLI requires --vertex --fragment --output");
        Require(!std::filesystem::exists(output), "output must be a fresh directory; original evidence is not overwritten");
        std::filesystem::create_directories(output);
        const auto vs = Shader(vertex), ps = Shader(fragment);
        state->Initialize();
        std::cout << "vertex_stride=28; attributes=int3@0,float3@12,direction@24; binding=b0:0,t0:1,s0:2; set=0\n"
                     "matrix=column-major,64bytes; sx=.25,sy=.35,tx=.08,ty=-.06,tz=.4; viewport=negative-height-only; front_face=counter-clockwise; cull=back; output_encode=UNORM-no-extra-SRGB\n";
        std::cout << "direction_shader_input=" << DirectionShaderInput << "; active_vertex_attributes=" << (DirectionShaderInput ? 3 : 2) << '\n';
        Draw(*state, vs, ps, false, output);
        Draw(*state, vs, ps, true, output);
        state->Close();
        NativeReport(*state, output);
        std::cout << "cleanup_completed=1; E02_E03_full_game_MSAA_GPU_timing=not-run\n";
    } catch (const std::exception& error) {
        std::cerr << "[Vulkan E01] " << error.what() << '\n'; result = 1;
        try { state->Close(); }
        catch (const std::exception& cleanup) { std::cerr << "[terminal retention] " << cleanup.what() << '\n'; }
    }
    std::cout << "validation_warnings=" << state->diagnostics.warnings.load() << "; validation_errors=" << state->diagnostics.errors.load() << '\n';
    if (state->diagnostics.warnings.load() || state->diagnostics.errors.load()) result = 1;
    std::cout << "e01_probe_exit_code=" << result << '\n';
    if (state->pending) { std::cout << "resources_retained_until_process_exit=1; cleanup_completed=0\n"; (void)state.release(); }
    return result;
}
