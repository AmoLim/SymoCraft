include_guard(GLOBAL)

function(symocraft_add_vulkan_backend)
    if(CMAKE_CONFIGURATION_TYPES)
        message(FATAL_ERROR "Vulkan uses separate Debug/Release single-configuration builds.")
    endif()
    find_path(SYMOCRAFT_VULKAN_HEADERS NAMES vulkan/vulkan_core.h
        HINTS "$ENV{VULKAN_SDK}/Include" DOC "Private Vulkan SDK headers" REQUIRED)
    find_program(SYMOCRAFT_VULKAN_DXC NAMES dxc.exe dxc
        HINTS "$ENV{VULKAN_SDK}/Bin" DOC "Separate SPIR-V-capable DXC; not the D3D12 compiler" REQUIRED)
    execute_process(COMMAND "${SYMOCRAFT_VULKAN_DXC}" -help
        OUTPUT_VARIABLE dxc_help ERROR_VARIABLE dxc_error RESULT_VARIABLE dxc_status TIMEOUT 30)
    if(NOT dxc_status EQUAL 0 OR NOT "${dxc_help}${dxc_error}" MATCHES "-spirv")
        message(FATAL_ERROR "SYMOCRAFT_VULKAN_DXC must support -spirv; the Windows SDK DXC may not.")
    endif()
    set(shader_source "${SYMOCRAFT_REPOSITORY_ROOT}/assets/shaders/hlsl")
    get_filename_component(spirv_compiler_directory "${SYMOCRAFT_VULKAN_DXC}" DIRECTORY)
    set(spirv_compiler_inputs "${SYMOCRAFT_VULKAN_DXC}" "${spirv_compiler_directory}/dxcompiler.dll")
    set(shader_output "${PROJECT_BINARY_DIR}/bin/assets/shaders/vulkan")
    set(shader_flags -nologo -Ges -WX -Zpc -spirv -fspv-target-env=vulkan1.2
        -fvk-use-dx-layout -fvk-b-shift 0 0 -fvk-t-shift 1 0 -fvk-s-shift 2 0)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND shader_flags -Od)
    else()
        list(APPEND shader_flags -O3)
    endif()
    # Some Windows SDK binaries advertise -spirv even with CodeGen disabled.
    set(probe_directory "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/vulkan-dxc-probe")
    file(MAKE_DIRECTORY "${probe_directory}")
    file(WRITE "${probe_directory}/probe.hlsl" "[numthreads(1, 1, 1)] void main() {}\n")
    execute_process(COMMAND "${SYMOCRAFT_VULKAN_DXC}" ${shader_flags} -E main -T cs_6_0
        -Fo "${probe_directory}/probe.spv" "${probe_directory}/probe.hlsl"
        OUTPUT_VARIABLE probe_output ERROR_VARIABLE probe_error RESULT_VARIABLE probe_status TIMEOUT 30)
    if(NOT probe_status EQUAL 0 OR NOT EXISTS "${probe_directory}/probe.spv")
        message(FATAL_ERROR "SYMOCRAFT_VULKAN_DXC failed the actual SPIR-V compile probe: ${probe_status}\n${probe_output}${probe_error}")
    endif()
    file(SIZE "${probe_directory}/probe.spv" probe_size)
    file(READ "${probe_directory}/probe.spv" probe_magic LIMIT 4 HEX)
    if(probe_size LESS 20 OR NOT probe_magic STREQUAL "03022307")
        message(FATAL_ERROR "SYMOCRAFT_VULKAN_DXC did not produce a SPIR-V module.")
    endif()
    foreach(shader IN ITEMS blocks selection)
        foreach(stage IN ITEMS vs ps)
            if(stage STREQUAL "vs")
                set(entry VSMain)
            else()
                set(entry PSMain)
            endif()
            set(output "${shader_output}/${shader}.${stage}.spv")
            add_custom_command(OUTPUT "${output}"
                COMMAND "${CMAKE_COMMAND}" -E make_directory "${shader_output}"
                COMMAND "${SYMOCRAFT_VULKAN_DXC}" ${shader_flags} -E ${entry} -T ${stage}_6_0
                    -I "${shader_source}" -Fo "${output}" "${shader_source}/${shader}.hlsl"
                DEPENDS "${shader_source}/${shader}.hlsl" "${shader_source}/draw_constants.hlsli"
                    ${spirv_compiler_inputs}
                VERBATIM)
            list(APPEND shader_outputs "${output}")
        endforeach()
    endforeach()
    string(JOIN " " shader_flags_text ${shader_flags})
    set(identity_output "${shader_output}/build.json")
    add_custom_command(OUTPUT "${identity_output}"
        COMMAND "${CMAKE_COMMAND}" "-DCOMPILER=${SYMOCRAFT_VULKAN_DXC}"
            "-DSOURCE_ROOT=${shader_source}" "-DOUTPUT_ROOT=${shader_output}"
            "-DCONFIGURATION=${CMAKE_BUILD_TYPE}" "-DFLAGS=${shader_flags_text}"
            -P "${SYMOCRAFT_REPOSITORY_ROOT}/cmake/WriteVulkanShaderIdentity.cmake"
        DEPENDS ${shader_outputs} ${spirv_compiler_inputs}
            "${SYMOCRAFT_REPOSITORY_ROOT}/cmake/WriteVulkanShaderIdentity.cmake"
        VERBATIM)
    add_library(symocraft_renderer_vulkan STATIC src/backend/vulkan/vulkan_backend.cpp ${shader_outputs} "${identity_output}")
    target_include_directories(symocraft_renderer_vulkan SYSTEM PRIVATE "${SYMOCRAFT_VULKAN_HEADERS}")
    target_include_directories(symocraft_renderer_vulkan PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/graphics_bridge")
    target_link_libraries(symocraft_renderer_vulkan PRIVATE symocraft_renderer_backend_contract symocraft_platform)
    target_compile_features(symocraft_renderer_vulkan PRIVATE cxx_std_20)
    target_compile_options(symocraft_renderer_vulkan PRIVATE /W4 /utf-8)
    target_compile_definitions(symocraft_renderer_vulkan PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(symocraft_renderer_vulkan PRIVATE SYMOCRAFT_VULKAN_DEBUG_ABI=1)
    endif()
    install(FILES ${shader_outputs} "${identity_output}" DESTINATION assets/shaders/vulkan)
endfunction()
