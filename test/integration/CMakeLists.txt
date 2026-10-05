if(TARGET SymoCraft)
    add_test(NAME assets.game_preflight COMMAND SymoCraft --check-assets)
    set_tests_properties(assets.game_preflight PROPERTIES
        WORKING_DIRECTORY "${SYMOCRAFT_REPOSITORY_ROOT}" TIMEOUT 15)
    add_test(NAME assets.game_package
        COMMAND "${CMAKE_COMMAND}"
            "-DGAME_EXECUTABLE=$<TARGET_FILE:SymoCraft>"
            "-DSOURCE_ASSETS=${SYMOCRAFT_REPOSITORY_ROOT}/assets"
            "-DPROBE_PARENT=${CMAKE_CURRENT_BINARY_DIR}/package-probes"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckAssetPackage.cmake")
    set_tests_properties(assets.game_package PROPERTIES TIMEOUT 45)
    add_test(NAME world.summary_cli COMMAND "${CMAKE_COMMAND}"
        "-DGAME_EXECUTABLE=$<TARGET_FILE:SymoCraft>"
        "-DOUTPUT_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/world-summary-probes"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckWorldSummary.cmake")
    set_tests_properties(world.summary_cli PROPERTIES TIMEOUT 240)
endif()
add_test(NAME architecture.source_boundaries COMMAND "${CMAKE_COMMAND}"
    "-DREPO=${SYMOCRAFT_REPOSITORY_ROOT}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckSourceBoundaries.cmake")
add_test(NAME architecture.negative_boundaries COMMAND "${CMAKE_COMMAND}"
    "-DREPO=${SYMOCRAFT_REPOSITORY_ROOT}"
    "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/negative-boundaries"
    "-DNINJA=${CMAKE_MAKE_PROGRAM}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckNegativeBoundaries.cmake")
set_tests_properties(architecture.source_boundaries architecture.negative_boundaries PROPERTIES TIMEOUT 60)
