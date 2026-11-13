symocraft_test_executable(profiling_tests profiling_tests.cpp symocraft_profiling)
target_sources(profiling_tests PRIVATE profiling_other.cpp)
add_test(NAME profiling.macros COMMAND profiling_tests)
set_tests_properties(profiling.macros PROPERTIES TIMEOUT 15)

# Always test disabled argument elimination, even in an enabled parent build.
symocraft_test_executable(profiling_disabled_tests profiling_tests.cpp)
target_sources(profiling_disabled_tests PRIVATE profiling_other.cpp)
target_include_directories(profiling_disabled_tests PRIVATE
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/profiling/include")
add_test(NAME profiling.disabled COMMAND profiling_disabled_tests)
set_tests_properties(profiling.disabled PROPERTIES TIMEOUT 15)

add_test(NAME profiling.configuration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/check_configuration.py"
    --repo "${SYMOCRAFT_REPOSITORY_ROOT}" --cmake "${CMAKE_COMMAND}"
    --ninja "${CMAKE_MAKE_PROGRAM}")
set_tests_properties(profiling.configuration PROPERTIES TIMEOUT 90)

symocraft_test_executable(gpu_markers_disabled_tests gpu_markers_off_tests.cpp)
target_include_directories(gpu_markers_disabled_tests PRIVATE
    "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src/backend")
add_test(NAME profiling.gpu_markers_disabled COMMAND gpu_markers_disabled_tests)
set_tests_properties(profiling.gpu_markers_disabled PROPERTIES TIMEOUT 15)

if(TARGET symocraft_renderer_vulkan)
    symocraft_test_executable(vulkan_debug_markers_tests vulkan_debug_markers_tests.cpp)
    target_include_directories(vulkan_debug_markers_tests PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src/backend/vulkan")
    target_include_directories(vulkan_debug_markers_tests SYSTEM PRIVATE "${SYMOCRAFT_VULKAN_HEADERS}")
    target_compile_definitions(vulkan_debug_markers_tests PRIVATE SYMOCRAFT_GPU_MARKERS_ENABLED=1)
    add_test(NAME profiling.vulkan_annotations COMMAND vulkan_debug_markers_tests)
    set_tests_properties(profiling.vulkan_annotations PROPERTIES TIMEOUT 15)
endif()
