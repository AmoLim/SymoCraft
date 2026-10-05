symocraft_test_executable(performance_tests performance_tests.cpp symocraft_telemetry symocraft_yaml)
add_test(NAME performance.export COMMAND performance_tests "${CMAKE_CURRENT_BINARY_DIR}/performance-probes")
set_tests_properties(performance.export PROPERTIES TIMEOUT 45)
