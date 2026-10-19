add_library(symocraft_startup STATIC src/startup_options.cpp)
target_compile_features(symocraft_startup PUBLIC cxx_std_20)
target_compile_options(symocraft_startup PRIVATE /W4 /utf-8)

add_library(symocraft_app_render_data STATIC src/world_render_cache.cpp src/render_conversion.cpp)
target_link_libraries(symocraft_app_render_data PUBLIC symocraft_renderer_api symocraft_world
    symocraft_assets symocraft_simulation)
target_compile_options(symocraft_app_render_data PRIVATE /W4 /utf-8)
target_compile_features(symocraft_app_render_data PUBLIC cxx_std_20)

if(NOT SYMOCRAFT_BUILD_GAME)
    return()
endif()

add_library(symocraft_app_runtime STATIC src/application.cpp src/game_renderer.cpp src/game_entry.cpp)
target_link_libraries(symocraft_app_runtime PUBLIC symocraft_startup symocraft_foundation symocraft_scene
    symocraft_assets symocraft_ecs symocraft_world symocraft_simulation symocraft_platform
    symocraft_renderer symocraft_app_render_data symocraft_telemetry)
target_compile_features(symocraft_app_runtime PUBLIC cxx_std_20)
target_compile_options(symocraft_app_runtime PRIVATE /W4 /utf-8)

add_executable(SymoCraft src/main.cpp)
target_link_libraries(SymoCraft PRIVATE symocraft_startup symocraft_foundation symocraft_scene
    symocraft_assets symocraft_ecs symocraft_world symocraft_simulation symocraft_platform
    symocraft_renderer symocraft_app_render_data symocraft_telemetry symocraft_app_runtime)
if(TARGET symocraft_renderer_legacy_opengl)
    target_link_libraries(symocraft_app_runtime PRIVATE symocraft_renderer_legacy_opengl)
    target_compile_definitions(symocraft_app_runtime PRIVATE SYMOCRAFT_APP_OPENGL=1)
endif()
if(TARGET symocraft_renderer_d3d12)
    target_compile_definitions(symocraft_app_runtime PRIVATE SYMOCRAFT_APP_D3D12=1)
    # Candidate provenance has stricter scope than ordinary preset builds.
    cmake_path(IS_PREFIX SYMOCRAFT_REPOSITORY_ROOT "${PROJECT_BINARY_DIR}" NORMALIZE in_repository)
    file(RELATIVE_PATH candidate_build_relative "${SYMOCRAFT_REPOSITORY_ROOT}" "${PROJECT_BINARY_DIR}")
    if(in_repository AND candidate_build_relative MATCHES "^out/m3-t3/" AND NOT SYMOCRAFT_BUILD_BENCHMARK)
        find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
        set(candidate_script "${SYMOCRAFT_REPOSITORY_ROOT}/scripts/package-d3d12-candidate.py")
        set_property(TARGET SymoCraft APPEND PROPERTY LINK_DEPENDS "${candidate_script}")
        add_custom_command(TARGET SymoCraft POST_BUILD
            COMMAND "${Python3_EXECUTABLE}" "${candidate_script}"
                --build-directory "${PROJECT_BINARY_DIR}" --record-build-identity
            COMMENT "Recording the production candidate executable and build identities"
            VERBATIM)
    endif()

    # Debug candidates are local developer diagnostics, never redistributable packages.
    get_filename_component(compiler_directory "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(toolset_directory "${compiler_directory}/../../.." ABSOLUTE)
    get_filename_component(toolset_version "${toolset_directory}" NAME)
    get_filename_component(vc_directory "${toolset_directory}/../../.." ABSOLUTE)
    set(candidate_redist "${vc_directory}/Redist/MSVC/${toolset_version}")
    if(NOT IS_DIRECTORY "${candidate_redist}/x64/Microsoft.VC143.CRT")
        message(FATAL_ERROR "D3D12 candidates require app-local CRT files matching the selected MSVC toolset: ${candidate_redist}")
    endif()
    set(MSVC_REDIST_DIR "${candidate_redist}" CACHE PATH "Selected MSVC toolset runtime directory" FORCE)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
        set(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)
        set(CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY TRUE)
        set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
        include(InstallRequiredSystemLibraries)
        foreach(runtime IN ITEMS msvcp140d.dll vcruntime140d.dll vcruntime140_1d.dll ucrtbased.dll)
            set(found_runtime FALSE)
            foreach(library IN LISTS CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
                get_filename_component(name "${library}" NAME)
                if(name STREQUAL runtime)
                    set(found_runtime TRUE)
                endif()
            endforeach()
            if(NOT found_runtime)
                message(FATAL_ERROR "Required matching Debug candidate runtime is missing: ${runtime}")
            endif()
        endforeach()
    endif()
endif()
if(TARGET symocraft_renderer_vulkan)
    target_compile_definitions(symocraft_app_runtime PRIVATE SYMOCRAFT_APP_VULKAN=1)
endif()
target_compile_features(SymoCraft PRIVATE cxx_std_20)
target_compile_options(SymoCraft PRIVATE /W4 /utf-8)
target_sources(SymoCraft PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/cmake/windows-utf8.manifest")
symocraft_stage_assets(SymoCraft)
install(TARGETS SymoCraft RUNTIME DESTINATION .)
install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/sdl3/LICENSE.txt" DESTINATION licenses RENAME SDL3-LICENSE.txt)
install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/stb/LICENSE" DESTINATION licenses RENAME stb-LICENSE.txt)
set(game_licenses GLM yaml-cpp Apache-2.0)
if(SYMOCRAFT_RENDER_OPENGL)
    list(APPEND game_licenses glad)
endif()
foreach(library IN LISTS game_licenses)
    install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/cmake/licenses/${library}-LICENSE.txt" DESTINATION licenses)
endforeach()
install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/cmake/licenses/SOURCES.txt" DESTINATION licenses)
install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/fast_noise_lite/FastNoiseLite.h"
    DESTINATION licenses RENAME FastNoiseLite-license-source.h)
if(SYMOCRAFT_RENDER_OPENGL)
    install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/KHR/khrplatform.h"
        DESTINATION licenses RENAME Khronos-license-source.h)
endif()
install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/robin_hood.h"
    DESTINATION licenses RENAME RobinHood-license-source.h)
install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/sdl3/src/hidapi/LICENSE.txt"
    DESTINATION licenses RENAME SDL3-hidapi-LICENSE.txt)
install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/sdl3/src/hidapi/LICENSE-bsd.txt"
    DESTINATION licenses RENAME SDL3-hidapi-BSD-LICENSE.txt)
install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/sdl3/src/video/yuv2rgb/LICENSE"
    DESTINATION licenses RENAME SDL3-yuv2rgb-LICENSE.txt)
foreach(source IN ITEMS src/libm/e_log.c src/stdlib/SDL_qsort.c src/stdlib/SDL_strtokr.c
        src/video/SDL_rotate.c src/video/stb_image.h src/video/miniz.h
        include/SDL3/SDL_opengl.h include/SDL3/SDL_opengl_glext.h
        src/video/khronos/vulkan/vk_platform.h src/video/khronos/vulkan/vulkan_win32.h)
    get_filename_component(name "${source}" NAME_WE)
    install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/sdl3/${source}"
        DESTINATION licenses RENAME "SDL3-${name}-license-source.txt")
endforeach()
foreach(asset IN LISTS SYMOCRAFT_REQUIRED_ASSETS)
    get_filename_component(directory "${asset}" DIRECTORY)
    install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/assets/${asset}" DESTINATION "assets/${directory}")
endforeach()
