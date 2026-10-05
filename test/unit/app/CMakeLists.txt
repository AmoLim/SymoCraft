symocraft_test_executable(startup_options_tests startup_options_tests.cpp symocraft_startup)
target_include_directories(startup_options_tests PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src")
add_test(NAME core.startup_options COMMAND startup_options_tests)
set_tests_properties(core.startup_options PROPERTIES TIMEOUT 45)
