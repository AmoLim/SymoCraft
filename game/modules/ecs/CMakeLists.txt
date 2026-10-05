add_library(symocraft_ecs STATIC src/registry.cpp src/internal.cpp)
target_include_directories(symocraft_ecs PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(symocraft_ecs PUBLIC symocraft_foundation)
target_compile_options(symocraft_ecs PRIVATE /utf-8 /W4)
