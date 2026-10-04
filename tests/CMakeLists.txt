add_executable(asset_paths_tests asset_paths_tests.cpp)
target_link_libraries(asset_paths_tests PRIVATE symocraft_assets)
if(MSVC)
    target_compile_options(asset_paths_tests PRIVATE /W4 /utf-8)
endif()
symocraft_stage_assets(asset_paths_tests)

add_test(NAME assets.contracts COMMAND asset_paths_tests)
add_test(NAME assets.from_source_directory COMMAND asset_paths_tests)
set_tests_properties(assets.from_source_directory PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
set_tests_properties(assets.contracts assets.from_source_directory PROPERTIES TIMEOUT 45)

add_test(NAME assets.game_preflight COMMAND SymoCraft --check-assets)
set_tests_properties(assets.game_preflight PROPERTIES
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    TIMEOUT 15)

add_test(NAME assets.game_package
    COMMAND "${CMAKE_COMMAND}"
        "-DGAME_EXECUTABLE=$<TARGET_FILE:SymoCraft>"
        "-DSOURCE_ASSETS=${PROJECT_SOURCE_DIR}/assets"
        "-DPROBE_PARENT=${CMAKE_CURRENT_BINARY_DIR}/package-probes"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckAssetPackage.cmake")
set_tests_properties(assets.game_package PROPERTIES TIMEOUT 45)

add_executable(shader_loading_tests shader_loading_tests.cpp
    ../src/renderer/shader.cpp
    ../src/renderer/shader_program.cpp)
target_link_libraries(shader_loading_tests PRIVATE symocraft_glad glm::glm symocraft_vendor_headers glfw)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/empty-shader.glsl" CONTENT "")

add_executable(player_math_tests player_math_tests.cpp)
target_link_libraries(player_math_tests PRIVATE glm::glm)

add_executable(batch_safety_tests batch_safety_tests.cpp)
target_link_libraries(batch_safety_tests PRIVATE symocraft_glad symocraft_gpu_timer glm::glm symocraft_vendor_headers glfw)

add_executable(mesh_safety_tests mesh_safety_tests.cpp
    ../src/world/chunk.cpp
    ../src/world/chunk_manager.cpp
    ../src/world/generation.cpp
    ../src/world/world_coordinates.cpp
    ../include/MemoryAllocator/AmoBase.cpp)
target_link_libraries(mesh_safety_tests PRIVATE symocraft_glad symocraft_gpu_timer glm::glm symocraft_vendor_headers glfw symocraft_yaml)

add_executable(block_config_tests block_config_tests.cpp ../src/world/block.cpp)
target_link_libraries(block_config_tests PRIVATE symocraft_yaml glm::glm symocraft_vendor_headers glfw)

add_executable(key_snapshot_tests key_snapshot_tests.cpp)
add_executable(startup_options_tests startup_options_tests.cpp ../src/core/startup_options.cpp)
add_executable(world_generation_tests world_generation_tests.cpp
    ../src/world/chunk.cpp ../src/world/chunk_manager.cpp ../src/world/block.cpp
    ../src/world/generation.cpp ../src/world/test_scene.cpp ../src/world/world_coordinates.cpp
    ../include/MemoryAllocator/AmoBase.cpp)
target_link_libraries(world_generation_tests PRIVATE symocraft_glad symocraft_gpu_timer glm::glm symocraft_vendor_headers glfw symocraft_yaml)

add_executable(performance_tests performance_tests.cpp ../src/core/performance.cpp)
target_link_libraries(performance_tests PRIVATE symocraft_yaml)
add_executable(gpu_timer_tests gpu_timer_tests.cpp)
target_link_libraries(gpu_timer_tests PRIVATE symocraft_gpu_timer)

foreach(test_target IN ITEMS shader_loading_tests player_math_tests batch_safety_tests mesh_safety_tests block_config_tests key_snapshot_tests startup_options_tests world_generation_tests performance_tests gpu_timer_tests)
    target_include_directories(${test_target} PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_compile_features(${test_target} PRIVATE cxx_std_20)
    target_compile_options(${test_target} PRIVATE /W4 /utf-8)
    set_target_properties(${test_target} PROPERTIES CXX_EXTENSIONS OFF)
endforeach()

add_test(NAME renderer.shader_loading COMMAND shader_loading_tests
    "${PROJECT_SOURCE_DIR}/assets/shaders/vs_BlockShader.glsl"
    "${PROJECT_SOURCE_DIR}/assets/shaders/fs_BlockShader.glsl"
    "${CMAKE_CURRENT_BINARY_DIR}/empty-shader.glsl")
add_test(NAME player.math COMMAND player_math_tests)
add_test(NAME renderer.batch_safety COMMAND batch_safety_tests)
add_test(NAME world.mesh_safety COMMAND mesh_safety_tests)
add_test(NAME world.block_config COMMAND block_config_tests "${PROJECT_SOURCE_DIR}/assets/configs/blockFormats.yaml")
add_test(NAME input.key_snapshot COMMAND key_snapshot_tests)
set_tests_properties(renderer.shader_loading player.math renderer.batch_safety world.mesh_safety world.block_config input.key_snapshot PROPERTIES TIMEOUT 45)
add_test(NAME core.startup_options COMMAND startup_options_tests)
add_test(NAME world.generation COMMAND world_generation_tests "${PROJECT_SOURCE_DIR}/assets/configs/blockFormats.yaml")
add_test(NAME world.summary_cli COMMAND "${CMAKE_COMMAND}"
    "-DGAME_EXECUTABLE=$<TARGET_FILE:SymoCraft>"
    "-DOUTPUT_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/world-summary-probes" -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckWorldSummary.cmake")
set_tests_properties(core.startup_options world.generation world.summary_cli PROPERTIES TIMEOUT 240)
add_test(NAME performance.export COMMAND performance_tests "${CMAKE_CURRENT_BINARY_DIR}/performance-probes")
add_test(NAME performance.gpu_timer COMMAND gpu_timer_tests)
set_tests_properties(performance.export performance.gpu_timer PROPERTIES TIMEOUT 45)

add_executable(benchmark_process_fixture benchmark_process_fixture.cpp)
target_compile_features(benchmark_process_fixture PRIVATE cxx_std_20)
target_compile_options(benchmark_process_fixture PRIVATE /W4 /utf-8)
set_target_properties(benchmark_process_fixture PROPERTIES CXX_EXTENSIONS OFF)
find_program(SYMOCRAFT_WINDOWS_POWERSHELL NAMES powershell.exe
    HINTS "$ENV{SystemRoot}/System32/WindowsPowerShell/v1.0" REQUIRED)
find_program(SYMOCRAFT_POWERSHELL_CORE NAMES pwsh.exe)
foreach(shell IN ITEMS WINDOWS_POWERSHELL POWERSHELL_CORE)
    if(SYMOCRAFT_${shell})
        add_test(NAME performance.script_${shell}
            COMMAND "${SYMOCRAFT_${shell}}" -NoProfile -NonInteractive -ExecutionPolicy Bypass
                -File "${CMAKE_CURRENT_SOURCE_DIR}/benchmark_script_tests.ps1"
                -BenchmarkScript "${PROJECT_SOURCE_DIR}/scripts/benchmark.ps1"
                -FixtureExecutable "$<TARGET_FILE:benchmark_process_fixture>"
                -OutputParent "${CMAKE_CURRENT_BINARY_DIR}/script-probes/${shell}")
        set_tests_properties(performance.script_${shell} PROPERTIES TIMEOUT 90)
    endif()
endforeach()
