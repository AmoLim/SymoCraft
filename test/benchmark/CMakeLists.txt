add_executable(benchmark_fixture "${SYMOCRAFT_REPOSITORY_ROOT}/test/support/benchmark_fixture.cpp"
    "${SYMOCRAFT_REPOSITORY_ROOT}/cmake/windows-utf8.manifest")
target_link_libraries(benchmark_fixture PRIVATE benchmark_core)
target_compile_options(benchmark_fixture PRIVATE /W4 /utf-8 /permissive-)
add_executable(benchmark_runner_tests runner_tests.cpp "${SYMOCRAFT_REPOSITORY_ROOT}/cmake/windows-utf8.manifest")
target_include_directories(benchmark_runner_tests PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/tools/benchmark/src")
target_link_libraries(benchmark_runner_tests PRIVATE benchmark_core)
target_compile_options(benchmark_runner_tests PRIVATE /W4 /utf-8 /permissive-)
foreach(suite IN ITEMS contracts process executor)
    add_test(NAME benchmark.${suite} COMMAND benchmark_runner_tests ${suite}
        "$<TARGET_FILE:benchmark_fixture>" "${CMAKE_CURRENT_BINARY_DIR}/probes")
    set_tests_properties(benchmark.${suite} PROPERTIES TIMEOUT 100)
endforeach()

add_executable(benchmark_process_fixture "${SYMOCRAFT_REPOSITORY_ROOT}/test/support/benchmark_process_fixture.cpp")
target_compile_features(benchmark_process_fixture PRIVATE cxx_std_20)
target_compile_options(benchmark_process_fixture PRIVATE /W4 /utf-8)
set_target_properties(benchmark_process_fixture PROPERTIES CXX_EXTENSIONS OFF)
find_program(SYMOCRAFT_WINDOWS_POWERSHELL NAMES powershell.exe
    HINTS "$ENV{SystemRoot}/System32/WindowsPowerShell/v1.0" REQUIRED)
find_program(SYMOCRAFT_POWERSHELL_CORE NAMES pwsh.exe)
foreach(shell IN ITEMS WINDOWS_POWERSHELL POWERSHELL_CORE)
    if(SYMOCRAFT_${shell})
        add_test(NAME performance.script_${shell}
            COMMAND "${SYMOCRAFT_${shell}}" -NoProfile -NonInteractive -ExecutionPolicy Bypass
                -File "${CMAKE_CURRENT_SOURCE_DIR}/benchmark_script_tests.ps1"
                -BenchmarkScript "${SYMOCRAFT_REPOSITORY_ROOT}/scripts/benchmark.ps1"
                -FixtureExecutable "$<TARGET_FILE:benchmark_process_fixture>"
                -OutputParent "${CMAKE_CURRENT_BINARY_DIR}/script-probes/${shell}")
        set_tests_properties(performance.script_${shell} PROPERTIES TIMEOUT 90)
    endif()
endforeach()
