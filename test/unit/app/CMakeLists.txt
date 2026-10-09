symocraft_test_executable(startup_options_tests startup_options_tests.cpp symocraft_startup)
target_include_directories(startup_options_tests PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src")
add_test(NAME core.startup_options COMMAND startup_options_tests)
set_tests_properties(core.startup_options PROPERTIES TIMEOUT 45)

symocraft_test_executable(cleanup_sequence_tests cleanup_sequence_tests.cpp)
target_include_directories(cleanup_sequence_tests PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src")
add_test(NAME core.cleanup_sequence COMMAND cleanup_sequence_tests)
set_tests_properties(core.cleanup_sequence PROPERTIES TIMEOUT 15)
