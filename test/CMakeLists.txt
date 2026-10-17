find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
add_test(NAME scripts.renderer_d3d12_frame_timings COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/integration/renderer/test_d3d12_frame_timings.py")
set_tests_properties(scripts.renderer_d3d12_frame_timings PROPERTIES TIMEOUT 30)
add_test(NAME scripts.renderer_vulkan_frame_timings COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/integration/renderer/test_vulkan_frame_timings.py")
set_tests_properties(scripts.renderer_vulkan_frame_timings PROPERTIES TIMEOUT 30)
add_test(NAME scripts.python_helpers COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_developer_scripts.py")
set_tests_properties(scripts.python_helpers PROPERTIES TIMEOUT 60)
add_test(NAME scripts.migrated_helpers COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_migrated_scripts.py")
set_tests_properties(scripts.migrated_helpers PROPERTIES TIMEOUT 120)
add_test(NAME scripts.renderer_draw_window COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_renderer_draw_window.py")
set_tests_properties(scripts.renderer_draw_window PROPERTIES TIMEOUT 30)
add_test(NAME scripts.renderer_vulkan_lifetime_fault COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_renderer_vulkan_lifetime_fault.py")
set_tests_properties(scripts.renderer_vulkan_lifetime_fault PROPERTIES TIMEOUT 30)
add_test(NAME scripts.renderer_vulkan_async COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_renderer_vulkan_async.py")
set_tests_properties(scripts.renderer_vulkan_async PROPERTIES TIMEOUT 30)
add_test(NAME scripts.renderer_d3d12_lifetime_async COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_renderer_d3d12_lifetime_async.py")
set_tests_properties(scripts.renderer_d3d12_lifetime_async PROPERTIES TIMEOUT 30)
add_test(NAME scripts.renderer_vulkan_wsi COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_renderer_vulkan_wsi.py")
set_tests_properties(scripts.renderer_vulkan_wsi PROPERTIES TIMEOUT 30)

function(symocraft_test_executable target source)
    add_executable(${target} "${source}")
    target_link_libraries(${target} PRIVATE ${ARGN})
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_compile_options(${target} PRIVATE /W4 /utf-8)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
endfunction()

if(TARGET symocraft_foundation)
    foreach(module IN ITEMS foundation assets ecs world simulation telemetry platform app)
        add_subdirectory("unit/${module}")
    endforeach()
    if(TARGET symocraft_renderer)
        add_subdirectory(unit/renderer)
    endif()
    include("${SYMOCRAFT_REPOSITORY_ROOT}/cmake/PublicHeaderTests.cmake")
    symocraft_public_header_tests()
    add_subdirectory(integration)
endif()
if(TARGET benchmark_core)
    add_subdirectory(benchmark)
endif()
