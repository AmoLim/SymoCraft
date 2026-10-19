if(TARGET SymoCraft)
    symocraft_test_executable(renderer_game_report_probe renderer/game_report_probe.cpp symocraft_telemetry)
    set(game_test_renderer opengl)
    if(NOT SYMOCRAFT_RENDER_OPENGL AND SYMOCRAFT_RENDER_D3D12)
        set(game_test_renderer d3d12)
    elseif(NOT SYMOCRAFT_RENDER_OPENGL)
        set(game_test_renderer vulkan)
    endif()
    add_test(NAME assets.game_preflight COMMAND SymoCraft --check-assets --renderer ${game_test_renderer})
    set_tests_properties(assets.game_preflight PROPERTIES
        WORKING_DIRECTORY "${SYMOCRAFT_REPOSITORY_ROOT}" TIMEOUT 15)
    add_test(NAME assets.game_package
        COMMAND "${CMAKE_COMMAND}"
            "-DGAME_EXECUTABLE=$<TARGET_FILE:SymoCraft>"
            "-DSOURCE_ASSETS=$<TARGET_FILE_DIR:SymoCraft>/assets"
            "-DSYMOCRAFT_RENDER_OPENGL=${SYMOCRAFT_RENDER_OPENGL}"
            "-DTEST_RENDERER=${game_test_renderer}"
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
if(TARGET symocraft_renderer)
    add_test(NAME architecture.renderer_boundaries COMMAND "${CMAKE_COMMAND}"
        "-DREPO=${SYMOCRAFT_REPOSITORY_ROOT}"
        "-DBUILD_DIR=${CMAKE_BINARY_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/CheckRendererBoundary.cmake")
    set_tests_properties(architecture.renderer_boundaries PROPERTIES TIMEOUT 45)
endif()
if(TARGET symocraft_renderer_d3d12 OR TARGET symocraft_renderer_vulkan)
    symocraft_test_executable(renderer_layer_summary_gpu_probe renderer/layer_summary_gpu_probe.cpp
        symocraft_renderer symocraft_platform symocraft_assets)
    set_target_properties(renderer_layer_summary_gpu_probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_dependencies(renderer_layer_summary_gpu_probe SymoCraft)
    # Explicit hardware verification only; not part of unattended CTest.
endif()
if(TARGET symocraft_renderer_vulkan)
    symocraft_test_executable(renderer_vulkan_frame_bindings_gpu_probe renderer/vulkan_frame_bindings_gpu_probe.cpp
        symocraft_renderer symocraft_platform symocraft_assets)
    target_include_directories(renderer_vulkan_frame_bindings_gpu_probe PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src/backend/vulkan"
        "${SYMOCRAFT_VULKAN_HEADERS}")
    target_compile_definitions(renderer_vulkan_frame_bindings_gpu_probe PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    set_target_properties(renderer_vulkan_frame_bindings_gpu_probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_dependencies(renderer_vulkan_frame_bindings_gpu_probe SymoCraft)
    symocraft_test_executable(renderer_vulkan_frame_timing_probe renderer/vulkan_frame_timing_probe.cpp
        symocraft_app_runtime symocraft_renderer symocraft_platform symocraft_telemetry)
    target_include_directories(renderer_vulkan_frame_timing_probe PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src"
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src"
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src/backend/vulkan"
        "${SYMOCRAFT_VULKAN_HEADERS}")
    target_compile_definitions(renderer_vulkan_frame_timing_probe PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    set_target_properties(renderer_vulkan_frame_timing_probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_dependencies(renderer_vulkan_frame_timing_probe SymoCraft)
    symocraft_test_executable(renderer_vulkan_wsi_probe renderer/vulkan_wsi_probe.cpp
        symocraft_renderer symocraft_platform symocraft_telemetry symocraft_assets)
    target_include_directories(renderer_vulkan_wsi_probe PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src/backend/vulkan"
        "${SYMOCRAFT_VULKAN_HEADERS}")
    target_compile_definitions(renderer_vulkan_wsi_probe PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    set_target_properties(renderer_vulkan_wsi_probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_dependencies(renderer_vulkan_wsi_probe SymoCraft)
    symocraft_test_executable(renderer_vulkan_lifetime_fault_probe renderer/vulkan_lifetime_fault_probe.cpp
        symocraft_app_runtime symocraft_renderer symocraft_platform symocraft_telemetry symocraft_assets)
    target_include_directories(renderer_vulkan_lifetime_fault_probe PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src"
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src/backend/vulkan"
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/graphics_bridge"
        "${SYMOCRAFT_VULKAN_HEADERS}")
    target_compile_definitions(renderer_vulkan_lifetime_fault_probe PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    set_target_properties(renderer_vulkan_lifetime_fault_probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_dependencies(renderer_vulkan_lifetime_fault_probe SymoCraft)
    symocraft_test_executable(renderer_draw_window_probe renderer/draw_window_probe.cpp
        symocraft_app_runtime symocraft_startup symocraft_renderer symocraft_platform symocraft_telemetry symocraft_assets symocraft_simulation)
    target_include_directories(renderer_draw_window_probe PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src"
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src/graphics_bridge")
    target_compile_definitions(renderer_draw_window_probe PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    set_target_properties(renderer_draw_window_probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_dependencies(renderer_draw_window_probe SymoCraft)
    symocraft_test_executable(renderer_vulkan_backend_smoke renderer/vulkan_backend_smoke.cpp
        symocraft_renderer symocraft_platform symocraft_assets)
    # Real windows/GPU diagnostics are executed explicitly, not by unattended CTest.
endif()
if(TARGET symocraft_renderer_d3d12)
    symocraft_test_executable(renderer_d3d12_srv_cache_gpu_probe renderer/d3d12_srv_cache_gpu_probe.cpp
        symocraft_renderer symocraft_platform symocraft_assets)
    target_include_directories(renderer_d3d12_srv_cache_gpu_probe PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src/backend/d3d12")
    target_compile_definitions(renderer_d3d12_srv_cache_gpu_probe PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    set_target_properties(renderer_d3d12_srv_cache_gpu_probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_dependencies(renderer_d3d12_srv_cache_gpu_probe SymoCraft)
    symocraft_test_executable(renderer_d3d12_frame_timing_probe renderer/d3d12_frame_timing_probe.cpp
        symocraft_app_runtime symocraft_renderer symocraft_platform symocraft_telemetry)
    target_include_directories(renderer_d3d12_frame_timing_probe PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src"
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src"
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src/backend/d3d12")
    target_compile_definitions(renderer_d3d12_frame_timing_probe PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    set_target_properties(renderer_d3d12_frame_timing_probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_dependencies(renderer_d3d12_frame_timing_probe SymoCraft)
    symocraft_test_executable(renderer_d3d12_lifetime_async_probe renderer/d3d12_lifetime_async_probe.cpp
        symocraft_renderer symocraft_platform symocraft_telemetry symocraft_assets)
    target_include_directories(renderer_d3d12_lifetime_async_probe PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src/backend/d3d12")
    target_compile_definitions(renderer_d3d12_lifetime_async_probe PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    set_target_properties(renderer_d3d12_lifetime_async_probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_dependencies(renderer_d3d12_lifetime_async_probe SymoCraft)
    symocraft_test_executable(renderer_d3d12_game_fault_probe renderer/d3d12_game_fault_probe.cpp
        symocraft_app_runtime symocraft_renderer symocraft_platform symocraft_telemetry)
    target_include_directories(renderer_d3d12_game_fault_probe PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/app/src"
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src/backend/d3d12")
    target_compile_definitions(renderer_d3d12_game_fault_probe PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_sources(renderer_d3d12_game_fault_probe PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/cmake/windows-utf8.manifest")
    set_target_properties(renderer_d3d12_game_fault_probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_dependencies(renderer_d3d12_game_fault_probe SymoCraft)
    symocraft_test_executable(renderer_d3d12_fault_probe renderer/d3d12_fault_probe.cpp
        symocraft_renderer symocraft_platform symocraft_telemetry)
    target_include_directories(renderer_d3d12_fault_probe PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src"
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/renderer/src/backend")
    target_compile_definitions(renderer_d3d12_fault_probe PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_sources(renderer_d3d12_fault_probe PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/cmake/windows-utf8.manifest")
    symocraft_test_executable(renderer_d3d12_backend_smoke
        renderer/d3d12_backend_smoke.cpp symocraft_renderer symocraft_platform
        symocraft_assets symocraft_foundation symocraft_stb_headers)
    target_include_directories(renderer_d3d12_backend_smoke PRIVATE
        "${SYMOCRAFT_REPOSITORY_ROOT}/game/modules/platform/src")
    target_compile_definitions(renderer_d3d12_backend_smoke PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    # Native hardware verification is explicit, never part of unattended CTest.
endif()
