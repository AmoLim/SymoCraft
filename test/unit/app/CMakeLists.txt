symocraft_test_executable(startup_options_tests startup_options_tests.cpp symocraft_startup)
target_include_directories(startup_options_tests PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src")
add_test(NAME core.startup_options COMMAND startup_options_tests)
set_tests_properties(core.startup_options PROPERTIES TIMEOUT 45)

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
symocraft_test_executable(app_scenario_tests scenario_tests.cpp symocraft_app_render_data)
target_include_directories(app_scenario_tests PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src")
add_test(NAME app.scenario_and_reports COMMAND app_scenario_tests "${SYMOCRAFT_REPOSITORY_ROOT}/assets/configs/blockFormats.yaml")
set_tests_properties(app.scenario_and_reports PROPERTIES TIMEOUT 45)
if(TARGET symocraft_application)
    symocraft_test_executable(app_profile_scenario_probe profile_scenario_probe.cpp
        symocraft_application symocraft_startup symocraft_simulation symocraft_telemetry SDL3::SDL3-static)
    target_compile_definitions(app_profile_scenario_probe PRIVATE SDL_MAIN_HANDLED)
    target_include_directories(app_profile_scenario_probe PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src")
    set_target_properties(app_profile_scenario_probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    symocraft_stage_assets(app_profile_scenario_probe)
    add_dependencies(app_profile_scenario_probe SymoCraft)
    # Explicit bounded native/window checks; never an unattended CTest GUI task.
    symocraft_test_executable(application_lifecycle_probe application_probe.cpp symocraft_application
        symocraft_startup symocraft_telemetry symocraft_foundation symocraft_document_io)
    target_include_directories(application_lifecycle_probe PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src")
    set_target_properties(application_lifecycle_probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    symocraft_stage_assets(application_lifecycle_probe)
    # Real backend/window work is explicit, not an unattended CPU test.
endif()
