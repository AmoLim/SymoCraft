add_library(symocraft_world STATIC
    src/block.cpp
    src/chunk.cpp
    src/world.cpp
    src/chunk_mesher.cpp
    src/generation.cpp
    src/test_scene.cpp
    src/world_coordinates.cpp)
target_compile_features(symocraft_world PUBLIC cxx_std_20)
target_include_directories(symocraft_world PUBLIC include PRIVATE src)
target_link_libraries(symocraft_world
    PUBLIC symocraft_foundation symocraft_scene
    PRIVATE symocraft_yaml symocraft_noise)
if(MSVC)
    target_compile_options(symocraft_world PRIVATE /W4 /utf-8)
endif()
