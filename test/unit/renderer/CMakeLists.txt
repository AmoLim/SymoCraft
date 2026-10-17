symocraft_test_executable(renderer_public_api_consumer public_api_consumer.cpp symocraft_renderer)
symocraft_test_executable(renderer_facade_tests facade_tests.cpp symocraft_renderer)
symocraft_test_executable(renderer_layer_summary_basic_tests layer_summary_basic_tests.cpp symocraft_renderer)
target_include_directories(renderer_layer_summary_basic_tests PRIVATE
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src")
add_test(NAME renderer.layer_summary_basic COMMAND renderer_layer_summary_basic_tests)
set_tests_properties(renderer.layer_summary_basic PROPERTIES TIMEOUT 15)
symocraft_test_executable(renderer_d3d12_srv_cache_basic_tests d3d12_srv_cache_basic_tests.cpp symocraft_renderer)
target_include_directories(renderer_d3d12_srv_cache_basic_tests PRIVATE
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src")
add_test(NAME renderer.d3d12_srv_cache_basic COMMAND renderer_d3d12_srv_cache_basic_tests)
set_tests_properties(renderer.d3d12_srv_cache_basic PROPERTIES TIMEOUT 15)
symocraft_test_executable(renderer_vulkan_frame_bindings_basic_tests vulkan_frame_bindings_basic_tests.cpp symocraft_renderer)
target_include_directories(renderer_vulkan_frame_bindings_basic_tests PRIVATE
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src")
add_test(NAME renderer.vulkan_frame_bindings_basic COMMAND renderer_vulkan_frame_bindings_basic_tests)
set_tests_properties(renderer.vulkan_frame_bindings_basic PROPERTIES TIMEOUT 15)
if(TARGET symocraft_renderer_vulkan)
    symocraft_test_executable(renderer_vulkan_buffer_basic_tests vulkan_buffer_basic_tests.cpp)
    target_include_directories(renderer_vulkan_buffer_basic_tests PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src")
    target_include_directories(renderer_vulkan_buffer_basic_tests SYSTEM PRIVATE "${SYMOCRAFT_VULKAN_HEADERS}")
    add_test(NAME renderer.vulkan_buffer_basic COMMAND renderer_vulkan_buffer_basic_tests)
    set_tests_properties(renderer.vulkan_buffer_basic PROPERTIES TIMEOUT 15)
endif()
symocraft_test_executable(renderer_frame_diagnostics_tests frame_diagnostics_tests.cpp)
target_include_directories(renderer_frame_diagnostics_tests PRIVATE
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src")
add_test(NAME renderer.frame_diagnostics COMMAND renderer_frame_diagnostics_tests)
set_tests_properties(renderer.frame_diagnostics PROPERTIES TIMEOUT 15)
target_include_directories(renderer_facade_tests PRIVATE
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src")
add_test(NAME renderer.public_api_consumer COMMAND renderer_public_api_consumer)
add_test(NAME renderer.facade_contract COMMAND renderer_facade_tests)
set_tests_properties(renderer.public_api_consumer renderer.facade_contract PROPERTIES TIMEOUT 45)

if(NOT TARGET symocraft_renderer_legacy_opengl)
    return()
endif()
symocraft_test_executable(shader_loading_tests shader_loading_tests.cpp symocraft_renderer_legacy_opengl symocraft_glad)
symocraft_test_executable(batch_safety_tests batch_safety_tests.cpp symocraft_renderer_legacy_opengl symocraft_glad symocraft_world)
symocraft_test_executable(gpu_timer_tests gpu_timer_tests.cpp symocraft_renderer_legacy_opengl symocraft_glad)
foreach(target IN ITEMS shader_loading_tests batch_safety_tests)
    # Existing GLAD function-pointer substitution is restricted to renderer white-box suites.
    target_include_directories(${target} PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src")
endforeach()
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/empty-shader.glsl" CONTENT "")
add_test(NAME renderer.shader_loading COMMAND shader_loading_tests
    "${SYMOCRAFT_REPOSITORY_ROOT}/assets/shaders/vs_BlockShader.glsl"
    "${SYMOCRAFT_REPOSITORY_ROOT}/assets/shaders/fs_BlockShader.glsl"
    "${CMAKE_CURRENT_BINARY_DIR}/empty-shader.glsl")
add_test(NAME renderer.batch_safety COMMAND batch_safety_tests
    "${SYMOCRAFT_REPOSITORY_ROOT}/assets/configs/blockFormats.yaml")
add_test(NAME performance.gpu_timer COMMAND gpu_timer_tests)
set_tests_properties(renderer.shader_loading renderer.batch_safety performance.gpu_timer PROPERTIES TIMEOUT 45)
