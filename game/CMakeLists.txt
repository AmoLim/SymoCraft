include("${PROJECT_SOURCE_DIR}/cmake/StageAssets.cmake")

set(BUILD_SHARED_LIBS OFF)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build GLFW examples" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build GLFW tests" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build GLFW documentation" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Install GLFW" FORCE)
add_subdirectory("${PROJECT_SOURCE_DIR}/vendor/glfw" "${PROJECT_BINARY_DIR}/vendor/glfw" EXCLUDE_FROM_ALL)

add_library(glm INTERFACE)
add_library(glm::glm ALIAS glm)
target_include_directories(glm SYSTEM INTERFACE "${PROJECT_SOURCE_DIR}/vendor")

add_library(symocraft_vendor_headers INTERFACE)
target_include_directories(symocraft_vendor_headers SYSTEM INTERFACE "${PROJECT_SOURCE_DIR}/vendor")

add_library(symocraft_glad STATIC "${PROJECT_SOURCE_DIR}/vendor/glad/glad.c")
target_include_directories(symocraft_glad SYSTEM PUBLIC "${PROJECT_SOURCE_DIR}/vendor")
target_compile_options(symocraft_glad PRIVATE /utf-8)

add_library(symocraft_gpu_timer STATIC "${PROJECT_SOURCE_DIR}/src/renderer/gpu_timer.cpp")
target_include_directories(symocraft_gpu_timer PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_features(symocraft_gpu_timer PUBLIC cxx_std_20)
target_compile_options(symocraft_gpu_timer PRIVATE /utf-8)
target_link_libraries(symocraft_gpu_timer PUBLIC symocraft_glad)

add_library(symocraft_assets STATIC "${PROJECT_SOURCE_DIR}/src/core/asset_paths.cpp")
target_include_directories(symocraft_assets PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_features(symocraft_assets PUBLIC cxx_std_20)
target_compile_options(symocraft_assets PRIVATE /utf-8)
set_target_properties(symocraft_assets PROPERTIES CXX_EXTENSIONS OFF)

add_executable(SymoCraft
    "${PROJECT_SOURCE_DIR}/src/main.cpp"
    "${PROJECT_SOURCE_DIR}/include/MemoryAllocator/AmoBase.cpp"
    "${PROJECT_SOURCE_DIR}/src/camera/Camera.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/application.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/startup_options.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/performance.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/global_thread_pool.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/window.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/ECS/internal.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/ECS/registry.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/ECS/Systems/character_system.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/ECS/Systems/physics_system.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/ECS/Systems/transform_system.cpp"
    "${PROJECT_SOURCE_DIR}/src/event/event.cpp"
    "${PROJECT_SOURCE_DIR}/src/input/input.cpp"
    "${PROJECT_SOURCE_DIR}/src/input/key_bindings.cpp"
    "${PROJECT_SOURCE_DIR}/src/input/key_handler.cpp"
    "${PROJECT_SOURCE_DIR}/src/playercontroller/playercontroller.cpp"
    "${PROJECT_SOURCE_DIR}/src/renderer/renderer.cpp"
    "${PROJECT_SOURCE_DIR}/src/renderer/performance_memory.cpp"
    "${PROJECT_SOURCE_DIR}/src/renderer/shader.cpp"
    "${PROJECT_SOURCE_DIR}/src/renderer/shader_program.cpp"
    "${PROJECT_SOURCE_DIR}/src/renderer/texture.cpp"
    "${PROJECT_SOURCE_DIR}/src/world/block.cpp"
    "${PROJECT_SOURCE_DIR}/src/world/chunk.cpp"
    "${PROJECT_SOURCE_DIR}/src/world/chunk_manager.cpp"
    "${PROJECT_SOURCE_DIR}/src/world/generation.cpp"
    "${PROJECT_SOURCE_DIR}/src/world/test_scene.cpp"
    "${PROJECT_SOURCE_DIR}/src/world/world_coordinates.cpp"
    "${PROJECT_SOURCE_DIR}/src/world/world.cpp"
)
target_include_directories(SymoCraft PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_compile_features(SymoCraft PRIVATE cxx_std_20)
target_compile_options(SymoCraft PRIVATE /utf-8)
set_target_properties(SymoCraft PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(SymoCraft PRIVATE
    glfw
    glm::glm
    symocraft_vendor_headers
    symocraft_glad
    symocraft_yaml
    symocraft_assets
    symocraft_gpu_timer
    Psapi
)
target_sources(SymoCraft PRIVATE "${PROJECT_SOURCE_DIR}/cmake/windows-utf8.manifest")
symocraft_stage_assets(SymoCraft)

if(BUILD_TESTING)
    add_subdirectory("${PROJECT_SOURCE_DIR}/tests" "${PROJECT_BINARY_DIR}/tests")
endif()

install(TARGETS SymoCraft RUNTIME DESTINATION .)
install(FILES "${PROJECT_SOURCE_DIR}/vendor/glfw/LICENSE.md" DESTINATION licenses RENAME GLFW-LICENSE.md)
install(FILES "${PROJECT_SOURCE_DIR}/vendor/stb/LICENSE" DESTINATION licenses RENAME STB-LICENSE.txt)
foreach(asset IN LISTS SYMOCRAFT_REQUIRED_ASSETS)
    get_filename_component(asset_directory "${asset}" DIRECTORY)
    install(FILES "${PROJECT_SOURCE_DIR}/assets/${asset}"
        DESTINATION "assets/${asset_directory}")
endforeach()
