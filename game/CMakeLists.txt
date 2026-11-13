include("${SYMOCRAFT_REPOSITORY_ROOT}/cmake/ThirdPartyGame.cmake")
include("${SYMOCRAFT_REPOSITORY_ROOT}/cmake/StageAssets.cmake")

add_subdirectory(modules/profiling)
foreach(module IN ITEMS foundation scene assets ecs world simulation telemetry)
    add_subdirectory("modules/${module}")
endforeach()
if(SYMOCRAFT_BUILD_GAME)
    symocraft_add_graphics_dependencies()
    add_subdirectory(modules/platform)
endif()
add_subdirectory(modules/renderer)
add_subdirectory(app)

# Only private implementation owners consume the optional instrumentation facade.
foreach(consumer IN ITEMS symocraft_world symocraft_simulation symocraft_app_render_data
        symocraft_application symocraft_renderer symocraft_renderer_legacy_opengl
        symocraft_renderer_d3d12 symocraft_renderer_vulkan)
    if(TARGET ${consumer})
        target_link_libraries(${consumer} PRIVATE symocraft_profiling)
    endif()
endforeach()

if(SYMOCRAFT_GPU_DEBUG_MARKERS)
    foreach(backend IN ITEMS symocraft_renderer_d3d12 symocraft_renderer_vulkan)
        if(TARGET ${backend})
            target_compile_definitions(${backend} PRIVATE SYMOCRAFT_GPU_MARKERS_ENABLED=1)
        endif()
    endforeach()
endif()
