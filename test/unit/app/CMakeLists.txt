symocraft_test_executable(startup_options_tests startup_options_tests.cpp symocraft_startup)
target_include_directories(startup_options_tests PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src")
add_test(NAME core.startup_options COMMAND startup_options_tests)
set_tests_properties(core.startup_options PROPERTIES TIMEOUT 45)

symocraft_test_executable(foreground_readiness_tests foreground_readiness_tests.cpp)
target_include_directories(foreground_readiness_tests PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src")
add_test(NAME app.foreground_readiness COMMAND foreground_readiness_tests)
set_tests_properties(app.foreground_readiness PROPERTIES TIMEOUT 15)

symocraft_test_executable(cleanup_sequence_tests cleanup_sequence_tests.cpp)
target_include_directories(cleanup_sequence_tests PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src")
add_test(NAME core.cleanup_sequence COMMAND cleanup_sequence_tests)
set_tests_properties(core.cleanup_sequence PROPERTIES TIMEOUT 15)

foreach(probe IN ITEMS cache conversion)
    symocraft_test_executable(app_render_${probe}_tests render_${probe}_tests.cpp symocraft_app_render_data)
    target_include_directories(app_render_${probe}_tests PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src")
    add_test(NAME app.render_${probe} COMMAND app_render_${probe}_tests
        "${SYMOCRAFT_REPOSITORY_ROOT}/assets/textures/texture_atlas.png")
    set_tests_properties(app.render_${probe} PROPERTIES TIMEOUT 45)
endforeach()
