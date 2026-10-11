symocraft_test_executable(performance_tests performance_tests.cpp symocraft_telemetry symocraft_yaml)
add_test(NAME performance.export COMMAND performance_tests "${CMAKE_CURRENT_BINARY_DIR}/performance-probes")
set_tests_properties(performance.export PROPERTIES TIMEOUT 45)

symocraft_test_executable(telemetry_renderer_tests renderer_performance_tests.cpp symocraft_telemetry symocraft_yaml)
add_test(NAME telemetry.renderer_performance COMMAND telemetry_renderer_tests
    "${CMAKE_CURRENT_BINARY_DIR}/renderer-performance-probes")
set_tests_properties(telemetry.renderer_performance PROPERTIES TIMEOUT 45)
