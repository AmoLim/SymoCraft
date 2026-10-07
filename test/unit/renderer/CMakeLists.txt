symocraft_test_executable(shader_loading_tests shader_loading_tests.cpp symocraft_renderer symocraft_glad)
symocraft_test_executable(batch_safety_tests batch_safety_tests.cpp symocraft_renderer symocraft_glad symocraft_world)
symocraft_test_executable(gpu_timer_tests gpu_timer_tests.cpp symocraft_renderer symocraft_glad)
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
