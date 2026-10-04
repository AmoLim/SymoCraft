cmake_minimum_required(VERSION 3.22)
project(SymoCraft VERSION 0.1.0 LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "SymoCraft currently supports Windows x64 only.")
endif()
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR NOT CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "SymoCraft requires the MSVC C and C++ toolchains. Select the Symocraft MSVC toolchain in CLion or run from an x64 Visual Studio developer environment.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "SymoCraft requires a 64-bit MSVC toolchain. Select the x64 target architecture.")
endif()
if(DEFINED MSVC_CXX_ARCHITECTURE_ID AND NOT MSVC_CXX_ARCHITECTURE_ID STREQUAL "x64")
    message(FATAL_ERROR "SymoCraft requires an x64 target, not ${MSVC_CXX_ARCHITECTURE_ID}.")
endif()

include(CTest)
include(cmake/ValidateMsvcEnvironment.cmake)
include(cmake/StageAssets.cmake)

set(BUILD_SHARED_LIBS OFF)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build GLFW examples" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build GLFW tests" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build GLFW documentation" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Install GLFW" FORCE)
add_subdirectory(vendor/glfw EXCLUDE_FROM_ALL)

add_library(glm INTERFACE)
add_library(glm::glm ALIAS glm)
target_include_directories(glm SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/vendor")

add_library(symocraft_vendor_headers INTERFACE)
target_include_directories(symocraft_vendor_headers SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/vendor")

add_library(symocraft_glad STATIC vendor/glad/glad.c)
target_include_directories(symocraft_glad SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor")
target_compile_options(symocraft_glad PRIVATE /utf-8)

add_library(symocraft_gpu_timer STATIC src/renderer/gpu_timer.cpp)
target_include_directories(symocraft_gpu_timer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(symocraft_gpu_timer PUBLIC cxx_std_20)
target_compile_options(symocraft_gpu_timer PRIVATE /utf-8)
target_link_libraries(symocraft_gpu_timer PUBLIC symocraft_glad)

add_library(symocraft_yaml STATIC
    vendor/yaml-cpp/src/binary.cpp
    vendor/yaml-cpp/src/convert.cpp
    vendor/yaml-cpp/src/depthguard.cpp
    vendor/yaml-cpp/src/directives.cpp
    vendor/yaml-cpp/src/emit.cpp
    vendor/yaml-cpp/src/emitfromevents.cpp
    vendor/yaml-cpp/src/emitter.cpp
    vendor/yaml-cpp/src/emitterstate.cpp
    vendor/yaml-cpp/src/emitterutils.cpp
    vendor/yaml-cpp/src/exceptions.cpp
    vendor/yaml-cpp/src/exp.cpp
    vendor/yaml-cpp/src/memory.cpp
    vendor/yaml-cpp/src/node.cpp
    vendor/yaml-cpp/src/node_data.cpp
    vendor/yaml-cpp/src/nodebuilder.cpp
    vendor/yaml-cpp/src/nodeevents.cpp
    vendor/yaml-cpp/src/null.cpp
    vendor/yaml-cpp/src/ostream_wrapper.cpp
    vendor/yaml-cpp/src/parse.cpp
    vendor/yaml-cpp/src/parser.cpp
    vendor/yaml-cpp/src/regex_yaml.cpp
    vendor/yaml-cpp/src/scanner.cpp
    vendor/yaml-cpp/src/scanscalar.cpp
    vendor/yaml-cpp/src/scantag.cpp
    vendor/yaml-cpp/src/scantoken.cpp
    vendor/yaml-cpp/src/simplekey.cpp
    vendor/yaml-cpp/src/singledocparser.cpp
    vendor/yaml-cpp/src/stream.cpp
    vendor/yaml-cpp/src/tag.cpp
)
target_include_directories(symocraft_yaml SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor")
target_compile_features(symocraft_yaml PUBLIC cxx_std_20)
target_compile_options(symocraft_yaml PRIVATE /utf-8)
set_target_properties(symocraft_yaml PROPERTIES CXX_EXTENSIONS OFF)

add_library(symocraft_assets STATIC src/core/asset_paths.cpp)
target_include_directories(symocraft_assets PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(symocraft_assets PUBLIC cxx_std_20)
target_compile_options(symocraft_assets PRIVATE /utf-8)
set_target_properties(symocraft_assets PROPERTIES CXX_EXTENSIONS OFF)

add_executable(SymoCraft
    src/main.cpp
    include/MemoryAllocator/AmoBase.cpp
    src/camera/Camera.cpp
    src/core/application.cpp
    src/core/startup_options.cpp
    src/core/performance.cpp
    src/core/global_thread_pool.cpp
    src/core/window.cpp
    src/core/ECS/internal.cpp
    src/core/ECS/registry.cpp
    src/core/ECS/Systems/character_system.cpp
    src/core/ECS/Systems/physics_system.cpp
    src/core/ECS/Systems/transform_system.cpp
    src/event/event.cpp
    src/input/input.cpp
    src/input/key_bindings.cpp
    src/input/key_handler.cpp
    src/playercontroller/playercontroller.cpp
    src/renderer/renderer.cpp
    src/renderer/performance_memory.cpp
    src/renderer/shader.cpp
    src/renderer/shader_program.cpp
    src/renderer/texture.cpp
    src/world/block.cpp
    src/world/chunk.cpp
    src/world/chunk_manager.cpp
    src/world/generation.cpp
    src/world/test_scene.cpp
    src/world/world_coordinates.cpp
    src/world/world.cpp
)
target_include_directories(SymoCraft PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
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
    add_subdirectory(tests)
endif()

install(TARGETS SymoCraft RUNTIME DESTINATION .)
foreach(asset IN LISTS SYMOCRAFT_REQUIRED_ASSETS)
    get_filename_component(asset_directory "${asset}" DIRECTORY)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/${asset}"
        DESTINATION "assets/${asset_directory}")
endforeach()

