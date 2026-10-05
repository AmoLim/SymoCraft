add_library(symocraft_simulation STATIC
    src/Camera.cpp
    src/character_system.cpp
    src/physics_system.cpp
    src/player.cpp
    src/playercontroller.cpp
    src/transform_system.cpp)
target_compile_features(symocraft_simulation PUBLIC cxx_std_20)
target_include_directories(symocraft_simulation PUBLIC include PRIVATE src)
target_link_libraries(symocraft_simulation
    PUBLIC symocraft_foundation symocraft_scene symocraft_ecs
    PRIVATE symocraft_world)
if(MSVC)
    target_compile_options(symocraft_simulation PRIVATE /W4 /utf-8)
endif()
