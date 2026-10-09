add_library(symocraft_platform STATIC src/window.cpp src/process_memory.cpp)
target_include_directories(symocraft_platform PUBLIC include PRIVATE src
    "${PROJECT_SOURCE_DIR}/vendor/sdl3/src/video/khronos")
target_link_libraries(symocraft_platform PUBLIC symocraft_foundation PRIVATE SDL3::SDL3-static Psapi)
target_compile_definitions(symocraft_platform PRIVATE SDL_MAIN_HANDLED)
target_compile_features(symocraft_platform PUBLIC cxx_std_20)
target_compile_options(symocraft_platform PRIVATE /W4 /utf-8)
