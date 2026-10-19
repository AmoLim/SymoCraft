symocraft_test_executable(key_snapshot_tests key_snapshot_tests.cpp)
# This pure input-value contract is usable without building any OS/window implementation.
target_include_directories(key_snapshot_tests PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/include")
add_test(NAME input.key_snapshot COMMAND key_snapshot_tests)
set_tests_properties(input.key_snapshot PROPERTIES TIMEOUT 45)
if(TARGET symocraft_platform)
    symocraft_test_executable(input_frame_tests input_frame_tests.cpp symocraft_platform SDL3::Headers)
    target_include_directories(input_frame_tests PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src")
    add_test(NAME input.owned_frame COMMAND input_frame_tests)
    set_tests_properties(input.owned_frame PROPERTIES TIMEOUT 45)
    symocraft_test_executable(platform_runtime_probe runtime_probe.cpp symocraft_platform SDL3::SDL3-static)
    target_compile_definitions(platform_runtime_probe PRIVATE SDL_MAIN_HANDLED)
    # Opens a real native window; run explicitly rather than in unattended CTest.
endif()
