symocraft_test_executable(key_snapshot_tests key_snapshot_tests.cpp)
# This pure input-value contract is usable without building any OS/window implementation.
target_include_directories(key_snapshot_tests PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/include")
add_test(NAME input.key_snapshot COMMAND key_snapshot_tests)
set_tests_properties(input.key_snapshot PROPERTIES TIMEOUT 45)
