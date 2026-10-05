add_library(symocraft_renderer STATIC
    src/renderer.cpp src/shader.cpp src/shader_program.cpp src/texture.cpp
    src/gpu_timer.cpp src/performance_memory.cpp)
target_include_directories(symocraft_renderer PUBLIC include PRIVATE src
    "${CMAKE_CURRENT_SOURCE_DIR}/../platform/src/graphics_bridge")
target_link_libraries(symocraft_renderer PUBLIC symocraft_foundation symocraft_scene
    PRIVATE symocraft_assets symocraft_platform symocraft_glad symocraft_stb_headers symocraft_robin_hood)
target_compile_features(symocraft_renderer PUBLIC cxx_std_20)
target_compile_options(symocraft_renderer PRIVATE /W4 /utf-8)
