include_guard(GLOBAL)

function(symocraft_add_d3d12_backend)
    include(CheckIncludeFileCXX)
    check_include_file_cxx(d3d12.h SYMOCRAFT_D3D12_HEADERS)
    check_include_file_cxx(dxgi1_6.h SYMOCRAFT_DXGI_HEADERS)
    if(NOT SYMOCRAFT_D3D12_HEADERS OR NOT SYMOCRAFT_DXGI_HEADERS)
        message(FATAL_ERROR "Enabled D3D12 backend requires Windows SDK D3D12 and DXGI 1.6 headers.")
    endif()
    foreach(library IN ITEMS d3d12 dxgi dxguid)
        string(TOUPPER "${library}" key)
        find_library(SYMOCRAFT_${key}_LIBRARY NAMES ${library}
            HINTS "$ENV{WindowsSdkDir}/Lib/$ENV{WindowsSDKLibVersion}/um/x64"
            PATHS ENV LIB REQUIRED)
        list(APPEND native_libraries "${SYMOCRAFT_${key}_LIBRARY}")
    endforeach()
    find_program(SYMOCRAFT_DXC NAMES dxc.exe dxc
        HINTS "$ENV{WindowsSdkDir}/bin/$ENV{WindowsSDKVersion}/x64"
        DOC "DXC used only at build time for D3D12 Shader Model 6.0 assets" REQUIRED)
    if(NOT EXISTS "${SYMOCRAFT_DXC}" OR IS_DIRECTORY "${SYMOCRAFT_DXC}")
        message(FATAL_ERROR "Enabled D3D12 backend requires an actual DXC executable, not FXC.")
    endif()
    set(shader_source "${SYMOCRAFT_REPOSITORY_ROOT}/assets/shaders/hlsl")
    get_filename_component(compiler_directory "${SYMOCRAFT_DXC}" DIRECTORY)
    set(compiler_inputs "${SYMOCRAFT_DXC}" "${compiler_directory}/dxcompiler.dll")
    set(shader_output "${PROJECT_BINARY_DIR}/bin/assets/shaders/d3d12")
    if(CMAKE_CONFIGURATION_TYPES)
        message(FATAL_ERROR "D3D12 validation currently uses separate Debug/Release single-configuration builds.")
    endif()
    set(shader_flags -nologo -Ges -WX -Zpc)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND shader_flags -Od -Zi -Qembed_debug)
    else()
        list(APPEND shader_flags -O3)
    endif()
    foreach(shader IN ITEMS blocks selection)
        foreach(stage IN ITEMS vs ps)
            if(stage STREQUAL "vs")
                set(entry VSMain)
            else()
                set(entry PSMain)
            endif()
            set(output "${shader_output}/${shader}.${stage}.dxil")
            add_custom_command(OUTPUT "${output}"
                COMMAND "${CMAKE_COMMAND}" -E make_directory "${shader_output}"
                COMMAND "${SYMOCRAFT_DXC}" ${shader_flags} -E ${entry} -T ${stage}_6_0
                    -I "${shader_source}" -Fo "${output}" "${shader_source}/${shader}.hlsl"
                DEPENDS "${shader_source}/${shader}.hlsl" "${shader_source}/draw_constants.hlsli"
                    ${compiler_inputs}
                COMMENT "Compiling D3D12 ${shader}.${stage} Shader Model 6.0"
                VERBATIM)
            list(APPEND shader_outputs "${output}")
        endforeach()
    endforeach()
    string(JOIN " " shader_flags_text ${shader_flags})
    set(identity_output "${shader_output}/build.json")
    add_custom_command(OUTPUT "${identity_output}"
        COMMAND "${CMAKE_COMMAND}" "-DCOMPILER=${SYMOCRAFT_DXC}" -DBACKEND=D3D12
            "-DSOURCE_ROOT=${shader_source}" "-DOUTPUT_ROOT=${shader_output}"
            "-DCONFIGURATION=${CMAKE_BUILD_TYPE}" "-DFLAGS=${shader_flags_text}"
            -P "${SYMOCRAFT_REPOSITORY_ROOT}/cmake/WriteVulkanShaderIdentity.cmake"
        DEPENDS ${shader_outputs} ${compiler_inputs}
            "${SYMOCRAFT_REPOSITORY_ROOT}/cmake/WriteVulkanShaderIdentity.cmake"
        VERBATIM)
    add_library(symocraft_renderer_d3d12 STATIC
        src/backend/d3d12/d3d12_backend.cpp src/backend/d3d12/pipeline.cpp ${shader_outputs} "${identity_output}")
    target_include_directories(symocraft_renderer_d3d12 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/backend/d3d12"
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/graphics_bridge")
    target_link_libraries(symocraft_renderer_d3d12 PRIVATE
        symocraft_renderer_backend_contract symocraft_platform ${native_libraries})
    target_compile_features(symocraft_renderer_d3d12 PRIVATE cxx_std_20)
    target_compile_options(symocraft_renderer_d3d12 PRIVATE /W4 /utf-8)
    target_compile_definitions(symocraft_renderer_d3d12 PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    set_target_properties(symocraft_renderer_d3d12 PROPERTIES CXX_EXTENSIONS OFF)
    install(FILES ${shader_outputs} "${identity_output}" DESTINATION assets/shaders/d3d12)
endfunction()
