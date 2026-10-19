cmake_minimum_required(VERSION 3.22)
project(SymoCraftPlatformSdl3Probe LANGUAGES C CXX)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The production platform probe requires Windows x64 and MSVC")
endif()
if(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
    message(FATAL_ERROR "Use Debug or Release matching the imported production libraries")
endif()

get_filename_component(SYMOCRAFT_REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(SYMOCRAFT_PLATFORM_PRODUCTION_BUILD "" CACHE PATH "Actual same-source production build directory")
option(SYMOCRAFT_PLATFORM_PROBE_VULKAN "Enable the real SDK-header Vulkan bridge probe" OFF)
option(SYMOCRAFT_PLATFORM_PROBE_INPUT "Enable production input and allocation regressions" OFF)
set(SYMOCRAFT_VULKAN_HEADERS_ROOT "" CACHE PATH "Explicit installed Vulkan SDK Include directory")
if(NOT EXISTS "${SYMOCRAFT_PLATFORM_PRODUCTION_BUILD}/CMakeCache.txt")
    message(FATAL_ERROR "Declare an existing production build; the probe never recompiles window.cpp or SDL")
endif()
file(STRINGS "${SYMOCRAFT_PLATFORM_PRODUCTION_BUILD}/CMakeCache.txt" production_cache)
foreach(entry IN LISTS production_cache)
    if(entry MATCHES "^CMAKE_BUILD_TYPE:STRING=(.*)$")
        set(production_configuration "${CMAKE_MATCH_1}")
    elseif(entry MATCHES "^CMAKE_HOME_DIRECTORY:INTERNAL=(.*)$")
        get_filename_component(production_source "${CMAKE_MATCH_1}" REALPATH)
    elseif(entry MATCHES "^SDL_STATIC:BOOL=(.*)$")
        set(production_static "${CMAKE_MATCH_1}")
    elseif(entry MATCHES "^SDL_SHARED:BOOL=(.*)$")
        set(production_shared "${CMAKE_MATCH_1}")
    endif()
endforeach()
get_filename_component(repository_source "${SYMOCRAFT_REPOSITORY_ROOT}" REALPATH)
if(NOT production_configuration STREQUAL CMAKE_BUILD_TYPE OR
   NOT production_source STREQUAL repository_source OR
   NOT production_static STREQUAL "ON" OR NOT production_shared STREQUAL "OFF")
    message(FATAL_ERROR "Imported libraries must be the matching static SDL production configuration from this repository")
endif()

set(identity "{\n  \"configuration\": \"${CMAKE_BUILD_TYPE}\",\n  \"production_build\": \"${SYMOCRAFT_PLATFORM_PRODUCTION_BUILD}\",\n  \"imports\": [")
set(separator "")
foreach(dependency IN ITEMS platform foundation sdl glad)
    if(dependency STREQUAL "sdl")
        set(archive "${SYMOCRAFT_PLATFORM_PRODUCTION_BUILD}/vendor/sdl3/SDL3-static.lib")
    elseif(dependency STREQUAL "glad")
        set(archive "${SYMOCRAFT_PLATFORM_PRODUCTION_BUILD}/game/symocraft_glad.lib")
    else()
        set(archive "${SYMOCRAFT_PLATFORM_PRODUCTION_BUILD}/game/modules/${dependency}/symocraft_${dependency}.lib")
    endif()
    if(NOT EXISTS "${archive}")
        message(FATAL_ERROR "Missing actual production library: ${archive}")
    endif()
    if(dependency STREQUAL "platform")
        file(GLOB_RECURSE platform_inputs
            "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/*.cpp"
            "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/*.h"
            "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/*.hpp"
            "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/include/*.h")
        foreach(input IN LISTS platform_inputs)
            if("${input}" IS_NEWER_THAN "${archive}")
                message(FATAL_ERROR "Production platform archive is stale for ${input}; build production before the probe")
            endif()
        endforeach()
    endif()
    add_library(production_${dependency} STATIC IMPORTED)
    set_target_properties(production_${dependency} PROPERTIES IMPORTED_LOCATION "${archive}")
    file(SHA256 "${archive}" digest)
    string(APPEND identity "${separator}\n    {\"target\": \"${dependency}\", \"archive\": \"${archive}\", \"sha256\": \"${digest}\"}")
    set(separator ",")
endforeach()
file(SHA256 "${SYMOCRAFT_PLATFORM_PRODUCTION_BUILD}/CMakeCache.txt" cache_digest)
file(SHA256 "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/window.cpp" window_digest)
file(SHA256 "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/input_state.h" input_digest)
string(APPEND identity "\n  ],\n  \"production_cache_sha256\": \"${cache_digest}\",\n  \"current_window_source_sha256\": \"${window_digest}\",\n  \"current_input_state_source_sha256\": \"${input_digest}\"\n}\n")
file(WRITE "${CMAKE_BINARY_DIR}/production-imports.json" "${identity}")

include("${SYMOCRAFT_REPOSITORY_ROOT}/cmake/ThirdPartyHeaders.cmake")
symocraft_header_view(platform_probe_glad_headers glad/glad.h KHR/khrplatform.h)
add_executable(SymoCraftPlatformSdl3Probe main.cpp)
target_compile_features(SymoCraftPlatformSdl3Probe PRIVATE cxx_std_20)
target_compile_options(SymoCraftPlatformSdl3Probe PRIVATE /W4 /utf-8)
target_compile_definitions(SymoCraftPlatformSdl3Probe PRIVATE SDL_MAIN_HANDLED)
target_include_directories(SymoCraftPlatformSdl3Probe PRIVATE
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/include"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/graphics_bridge"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/sdl3/include")
target_link_libraries(SymoCraftPlatformSdl3Probe PRIVATE
    production_platform production_foundation production_sdl production_glad platform_probe_glad_headers
    kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid advapi32 setupapi shell32 Psapi)

if(SYMOCRAFT_PLATFORM_PROBE_INPUT)
    foreach(target IN ITEMS SymoCraftPlatformInputTests SymoCraftPlatformAllocationTests)
        if(target STREQUAL "SymoCraftPlatformInputTests")
            set(source input_tests.cpp)
        else()
            set(source allocation_tests.cpp)
        endif()
        add_executable(${target} ${source})
        target_compile_features(${target} PRIVATE cxx_std_20)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
        target_compile_definitions(${target} PRIVATE SDL_MAIN_HANDLED)
        target_include_directories(${target} PRIVATE
            "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/include"
            "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/graphics_bridge"
            "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/sdl3/include")
        target_link_libraries(${target} PRIVATE
            production_platform production_foundation production_sdl production_glad
            kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid advapi32 setupapi shell32 Psapi)
    endforeach()
    # Compile the actual private reducer header without SDL initialization or a second window.cpp.
    add_executable(SymoCraftPlatformInputStateTests input_state_tests.cpp)
    target_compile_features(SymoCraftPlatformInputStateTests PRIVATE cxx_std_20)
    target_compile_options(SymoCraftPlatformInputStateTests PRIVATE /W4 /utf-8)
    target_include_directories(SymoCraftPlatformInputStateTests PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/include"
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src"
        "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/sdl3/include")
    include(CTest)
    if(BUILD_TESTING)
        add_test(NAME platform.input_state COMMAND SymoCraftPlatformInputStateTests)
        add_executable(SymoCraftPlatformInputOptionsTests input_options_tests.cpp)
        target_compile_features(SymoCraftPlatformInputOptionsTests PRIVATE cxx_std_20)
        target_compile_options(SymoCraftPlatformInputOptionsTests PRIVATE /W4 /utf-8)
        add_test(NAME platform.input_options COMMAND SymoCraftPlatformInputOptionsTests)
        set_tests_properties(platform.input_options PROPERTIES TIMEOUT 15)
    endif()
endif()

# SDL's dynamic API override is confined to this test executable and fixture DLL.
target_link_options(SymoCraftPlatformSdl3Probe PRIVATE /EXPORT:SDL_DYNAPI_entry)
add_library(symocraft_platform_failure_fixture SHARED failure_fixture.cpp)
target_compile_features(symocraft_platform_failure_fixture PRIVATE cxx_std_20)
target_compile_options(symocraft_platform_failure_fixture PRIVATE /W4 /utf-8)
target_include_directories(symocraft_platform_failure_fixture PRIVATE
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/sdl3/include"
    "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/sdl3/src")

if(SYMOCRAFT_PLATFORM_PROBE_VULKAN)
    if(NOT EXISTS "${SYMOCRAFT_VULKAN_HEADERS_ROOT}/vulkan/vulkan_core.h")
        message(FATAL_ERROR "The Vulkan probe requires an explicit installed SDK Include directory")
    endif()
    target_include_directories(SymoCraftPlatformSdl3Probe SYSTEM BEFORE PRIVATE
        "${SYMOCRAFT_VULKAN_HEADERS_ROOT}")
    target_compile_definitions(SymoCraftPlatformSdl3Probe PRIVATE SYMOCRAFT_PLATFORM_PROBE_VULKAN=1)
    add_executable(SymoCraftVulkanCapabilitiesProbe vulkan_capabilities.cpp)
    target_compile_features(SymoCraftVulkanCapabilitiesProbe PRIVATE cxx_std_20)
    target_compile_options(SymoCraftVulkanCapabilitiesProbe PRIVATE /W4 /utf-8)
    target_compile_definitions(SymoCraftVulkanCapabilitiesProbe PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_include_directories(SymoCraftVulkanCapabilitiesProbe SYSTEM BEFORE PRIVATE
        "${SYMOCRAFT_VULKAN_HEADERS_ROOT}")
    target_include_directories(SymoCraftVulkanCapabilitiesProbe PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/include"
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/graphics_bridge")
    target_link_libraries(SymoCraftVulkanCapabilitiesProbe PRIVATE
        production_platform production_foundation production_sdl production_glad
        kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid advapi32 setupapi shell32 Psapi)
    option(SYMOCRAFT_PLATFORM_PROBE_TEXTURED "Build the isolated Vulkan R1-E01 textured experiment" OFF)
    option(SYMOCRAFT_PLATFORM_PROBE_LIFETIME "Build the isolated Vulkan R1-E02 lifetime experiment" OFF)
    option(SYMOCRAFT_PLATFORM_PROBE_RESIZE "Build the isolated Vulkan R1-E03 presentation experiment" OFF)
    if((SYMOCRAFT_PLATFORM_PROBE_LIFETIME OR SYMOCRAFT_PLATFORM_PROBE_RESIZE) AND NOT SYMOCRAFT_PLATFORM_PROBE_TEXTURED)
        message(FATAL_ERROR "The E02/E03 experiments require the explicit textured shader experiment configuration")
    endif()
    if(SYMOCRAFT_PLATFORM_PROBE_TEXTURED)
        set(SYMOCRAFT_VULKAN_E01_DXC "" CACHE FILEPATH "Explicit SDK DXC executable for the isolated E01 experiment")
        if(NOT EXISTS "${SYMOCRAFT_VULKAN_E01_DXC}" OR IS_DIRECTORY "${SYMOCRAFT_VULKAN_E01_DXC}")
            message(FATAL_ERROR "The textured E01 experiment requires its explicit SDK DXC executable")
        endif()
        set(e01_shader_source "${SYMOCRAFT_REPOSITORY_ROOT}/assets/shaders/hlsl")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(e01_shader_optimization -Od)
        else()
            set(e01_shader_optimization -O3)
        endif()
        foreach(stage IN ITEMS vs ps)
            if(stage STREQUAL "vs")
                set(entry VSMain)
            else()
                set(entry PSMain)
            endif()
            set(e01_shader "${CMAKE_BINARY_DIR}/shaders/blocks.${stage}.spv")
            add_custom_command(OUTPUT "${e01_shader}"
                COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/shaders"
                COMMAND "${SYMOCRAFT_VULKAN_E01_DXC}" -nologo -Ges -WX -Zpc ${e01_shader_optimization} -spirv
                    -fspv-target-env=vulkan1.2 -fvk-use-dx-layout
                    -fvk-b-shift 0 0 -fvk-t-shift 1 0 -fvk-s-shift 2 0
                    -E ${entry} -T ${stage}_6_0 -I "${e01_shader_source}"
                    -Fo "${e01_shader}" "${e01_shader_source}/blocks.hlsl"
                DEPENDS "${e01_shader_source}/blocks.hlsl" "${e01_shader_source}/draw_constants.hlsli"
                VERBATIM)
            list(APPEND e01_shaders "${e01_shader}")
        endforeach()
        add_executable(SymoCraftVulkanTexturedProbe vulkan_textured_probe.cpp)
        set(textured_probes SymoCraftVulkanTexturedProbe)
        if(SYMOCRAFT_PLATFORM_PROBE_LIFETIME)
            add_executable(SymoCraftVulkanLifetimeProbe vulkan_lifetime_probe.cpp)
            list(APPEND textured_probes SymoCraftVulkanLifetimeProbe)
        endif()
        if(SYMOCRAFT_PLATFORM_PROBE_RESIZE)
            add_executable(SymoCraftVulkanResizeProbe vulkan_resize_probe.cpp)
            list(APPEND textured_probes SymoCraftVulkanResizeProbe)
        endif()
        foreach(probe IN LISTS textured_probes)
            target_sources(${probe} PRIVATE ${e01_shaders})
            target_compile_features(${probe} PRIVATE cxx_std_20)
            target_compile_options(${probe} PRIVATE /W4 /utf-8)
            target_compile_definitions(${probe} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                target_compile_definitions(${probe} PRIVATE SYMOCRAFT_VULKAN_E01_DEBUG_ABI=1)
            endif()
            target_include_directories(${probe} SYSTEM BEFORE PRIVATE
                "${SYMOCRAFT_VULKAN_HEADERS_ROOT}")
            target_include_directories(${probe} PRIVATE
                "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/include"
                "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/graphics_bridge")
            target_link_libraries(${probe} PRIVATE
                production_platform production_foundation production_sdl production_glad
                kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid advapi32 setupapi shell32 Psapi)
        endforeach()
    endif()
    message(STATUS "Production bridge probe Vulkan header source: ${SYMOCRAFT_VULKAN_HEADERS_ROOT}")
endif()

# Real windows are exercised only by explicit verification, never the pure CTest suite.
