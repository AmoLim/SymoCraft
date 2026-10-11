cmake_minimum_required(VERSION 3.22)
project(SymoCraftD3D12R1Experiment LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The isolated D3D12 experiment requires Windows x64 and MSVC")
endif()
if(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
    message(FATAL_ERROR "Use Debug or Release matching the imported production libraries")
endif()

get_filename_component(SYMOCRAFT_REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(SYMOCRAFT_R1_PRODUCTION_BUILD "" CACHE PATH "Independent same-source production platform build")
set(SYMOCRAFT_R1_DXC "" CACHE FILEPATH "Explicit DXC executable; FXC is not a substitute")
if(NOT EXISTS "${SYMOCRAFT_R1_DXC}" OR IS_DIRECTORY "${SYMOCRAFT_R1_DXC}")
    message(FATAL_ERROR "Pass SYMOCRAFT_R1_DXC pointing to an installed dxc.exe; shader model 6.0 is required")
endif()
if(NOT EXISTS "${SYMOCRAFT_R1_PRODUCTION_BUILD}/CMakeCache.txt")
    message(FATAL_ERROR "Build the actual production platform in an independent directory first")
endif()
file(STRINGS "${SYMOCRAFT_R1_PRODUCTION_BUILD}/CMakeCache.txt" production_cache)
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
    message(FATAL_ERROR "Imports must be matching static SDL production libraries built from this repository")
endif()

set(identity "{\n  \"configuration\": \"${CMAKE_BUILD_TYPE}\",\n  \"production_build\": \"${SYMOCRAFT_R1_PRODUCTION_BUILD}\",\n  \"imports\": [")
set(separator "")
foreach(dependency IN ITEMS platform foundation sdl)
    if(dependency STREQUAL "sdl")
        set(archive "${SYMOCRAFT_R1_PRODUCTION_BUILD}/vendor/sdl3/SDL3-static.lib")
    else()
        set(archive "${SYMOCRAFT_R1_PRODUCTION_BUILD}/game/modules/${dependency}/symocraft_${dependency}.lib")
    endif()
    if(NOT EXISTS "${archive}")
        message(FATAL_ERROR "Missing actual production archive: ${archive}")
    endif()
    add_library(production_${dependency} STATIC IMPORTED)
    set_target_properties(production_${dependency} PROPERTIES IMPORTED_LOCATION "${archive}")
    file(SHA256 "${archive}" digest)
    string(APPEND identity "${separator}\n    {\"target\": \"${dependency}\", \"archive\": \"${archive}\", \"sha256\": \"${digest}\"}")
    set(separator ",")
endforeach()
file(GLOB_RECURSE platform_inputs CONFIGURE_DEPENDS
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/*.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/*.h"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/*.hpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/include/*.h")
string(APPEND identity "\n  ],\n  \"platform_sources\": [")
set(separator "")
foreach(input IN LISTS platform_inputs)
    if("${input}" IS_NEWER_THAN "${SYMOCRAFT_R1_PRODUCTION_BUILD}/game/modules/platform/symocraft_platform.lib")
        message(FATAL_ERROR "Production platform archive is stale for ${input}; rebuild it before the experiment")
    endif()
    file(SHA256 "${input}" digest)
    string(APPEND identity "${separator}\n    {\"path\": \"${input}\", \"sha256\": \"${digest}\"}")
    set(separator ",")
endforeach()
file(GLOB_RECURSE foundation_inputs CONFIGURE_DEPENDS
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/foundation/src/*.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/foundation/src/*.h"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/foundation/src/*.hpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/foundation/src/*.inl"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/foundation/include/*.h"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/foundation/include/*.hpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/foundation/include/*.inl")
string(APPEND identity "\n  ],\n  \"foundation_sources\": [")
set(separator "")
foreach(input IN LISTS foundation_inputs)
    if("${input}" IS_NEWER_THAN "${SYMOCRAFT_R1_PRODUCTION_BUILD}/game/modules/foundation/symocraft_foundation.lib")
        message(FATAL_ERROR "Production foundation archive is stale for ${input}; rebuild it before the experiment")
    endif()
    file(SHA256 "${input}" digest)
    string(APPEND identity "${separator}\n    {\"path\": \"${input}\", \"sha256\": \"${digest}\"}")
    set(separator ",")
endforeach()
file(SHA256 "${SYMOCRAFT_R1_PRODUCTION_BUILD}/CMakeCache.txt" cache_digest)
file(SHA256 "${SYMOCRAFT_R1_DXC}" dxc_digest)
string(APPEND identity "\n  ],\n  \"production_cache_sha256\": \"${cache_digest}\",\n  \"dxc\": \"${SYMOCRAFT_R1_DXC}\",\n  \"dxc_sha256\": \"${dxc_digest}\",\n  \"shader_model\": \"6.0\"\n}\n")
file(WRITE "${CMAKE_BINARY_DIR}/production-imports.json" "${identity}")

include(CheckIncludeFileCXX)
check_include_file_cxx(d3d12.h SYMOCRAFT_R1_HAS_D3D12_HEADERS)
check_include_file_cxx(dxgi1_6.h SYMOCRAFT_R1_HAS_DXGI_HEADERS)
if(NOT SYMOCRAFT_R1_HAS_D3D12_HEADERS OR NOT SYMOCRAFT_R1_HAS_DXGI_HEADERS)
    message(FATAL_ERROR "The active Windows SDK must provide D3D12 and DXGI 1.6 headers")
endif()
foreach(library IN ITEMS d3d12 dxgi dxguid)
    find_library(SYMOCRAFT_R1_${library}_LIBRARY NAMES ${library}
        HINTS "$ENV{WindowsSdkDir}/Lib/$ENV{WindowsSDKLibVersion}/um/x64" PATHS ENV LIB REQUIRED)
endforeach()

set(shader_directory "${CMAKE_BINARY_DIR}/shaders")
foreach(stage IN ITEMS vs ps)
    if(stage STREQUAL "vs")
        set(entry VSMain)
    else()
        set(entry PSMain)
    endif()
    add_custom_command(OUTPUT "${shader_directory}/probe-${stage}.cso"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${shader_directory}"
        COMMAND "${SYMOCRAFT_R1_DXC}" -nologo -Ges -WX -E ${entry} -T ${stage}_6_0
            -Fo "${shader_directory}/probe-${stage}.cso" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/probe.hlsl"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/probe.hlsl"
        VERBATIM)
endforeach()
add_custom_target(symocraft_r1_shaders DEPENDS
    "${shader_directory}/probe-vs.cso" "${shader_directory}/probe-ps.cso")

include("${SYMOCRAFT_REPOSITORY_ROOT}/cmake/ThirdPartyHeaders.cmake")
symocraft_header_view(r1_image_writer stb/stb_image_write.h)
symocraft_header_view(r1_math glm)
add_executable(SymoCraftD3D12R1Probe probe.cpp renderer_probe.cpp)
add_dependencies(SymoCraftD3D12R1Probe symocraft_r1_shaders)
target_compile_features(SymoCraftD3D12R1Probe PRIVATE cxx_std_20)
target_compile_options(SymoCraftD3D12R1Probe PRIVATE /W4 /utf-8)
target_compile_definitions(SymoCraftD3D12R1Probe PRIVATE SDL_MAIN_HANDLED
    "SYMOCRAFT_R1_SHADER_DIRECTORY=\"${shader_directory}\"")
target_include_directories(SymoCraftD3D12R1Probe PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/contract"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/include"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/graphics_bridge"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/scene/include"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/foundation/include")
target_link_libraries(SymoCraftD3D12R1Probe PRIVATE
    production_platform production_foundation production_sdl r1_image_writer r1_math
    "${SYMOCRAFT_R1_d3d12_LIBRARY}" "${SYMOCRAFT_R1_dxgi_LIBRARY}" "${SYMOCRAFT_R1_dxguid_LIBRARY}"
    kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid advapi32 setupapi shell32 Psapi)

# Candidate consumers intentionally have neither SDK includes nor GPU-library linkage.
add_executable(SymoCraftD3D12R1CandidateTests contract/candidate_tests.cpp)
target_compile_features(SymoCraftD3D12R1CandidateTests PRIVATE cxx_std_20)
target_compile_options(SymoCraftD3D12R1CandidateTests PRIVATE /W4 /utf-8)
target_include_directories(SymoCraftD3D12R1CandidateTests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/contract"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/scene/include"
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/foundation/include")
target_link_libraries(SymoCraftD3D12R1CandidateTests PRIVATE r1_math)
include(CTest)
if(BUILD_TESTING)
    add_test(NAME r1.candidate_contract COMMAND SymoCraftD3D12R1CandidateTests)
endif()
# Real hardware runs remain explicit and separate from CTest.

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Only the isolated fixture is deployed; no production game or compiler is packaged.
    install(TARGETS SymoCraftD3D12R1Probe RUNTIME DESTINATION . COMPONENT R1Fixture)
    install(FILES "${shader_directory}/probe-vs.cso" "${shader_directory}/probe-ps.cso"
        DESTINATION shaders COMPONENT R1Fixture)
    install(FILES "${CMAKE_BINARY_DIR}/build-identities.json" "${CMAKE_BINARY_DIR}/production-imports.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/verify-probes.ps1" DESTINATION . COMPONENT R1Fixture)
    install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/sdl3/LICENSE.txt"
        DESTINATION licenses RENAME SDL3-LICENSE.txt COMPONENT R1Fixture)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT R1Fixture)
    set(CMAKE_INSTALL_DEBUG_LIBRARIES FALSE)
    set(CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY FALSE)
    set(CMAKE_INSTALL_UCRT_LIBRARIES FALSE)
    include(InstallRequiredSystemLibraries)
    if(NOT CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        message(FATAL_ERROR "The Release fixture requires discoverable app-local MSVC redistributable libraries")
    endif()
endif()
