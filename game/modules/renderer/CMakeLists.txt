add_library(symocraft_renderer_api INTERFACE)
target_include_directories(symocraft_renderer_api INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(symocraft_renderer_api INTERFACE symocraft_scene)
target_compile_features(symocraft_renderer_api INTERFACE cxx_std_20)

add_library(symocraft_renderer_backend_contract INTERFACE)
target_include_directories(symocraft_renderer_backend_contract INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/backend")
target_link_libraries(symocraft_renderer_backend_contract INTERFACE symocraft_renderer_api)

add_library(symocraft_renderer STATIC
    src/renderer.cpp src/renderer_impl.cpp src/resource_registry.cpp
    src/validation.cpp src/error.cpp src/backend/backend_factory.cpp)
target_include_directories(symocraft_renderer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(symocraft_renderer PUBLIC symocraft_renderer_api
    PRIVATE symocraft_renderer_backend_contract)
target_compile_options(symocraft_renderer PRIVATE /W4 /utf-8)
set_target_properties(symocraft_renderer PROPERTIES CXX_EXTENSIONS OFF)

if(SYMOCRAFT_RENDER_D3D12)
    include("${SYMOCRAFT_REPOSITORY_ROOT}/cmake/RendererD3D12.cmake")
    symocraft_add_d3d12_backend()
    target_link_libraries(symocraft_renderer PRIVATE symocraft_renderer_d3d12)
    target_compile_definitions(symocraft_renderer PRIVATE SYMOCRAFT_HAS_D3D12_BACKEND=1)
endif()

# Keep the isolated OpenGL comparison path while modern gameplay is being verified.
if(SYMOCRAFT_RENDER_VULKAN)
    include("${SYMOCRAFT_REPOSITORY_ROOT}/cmake/RendererVulkan.cmake")
    symocraft_add_vulkan_backend()
    target_link_libraries(symocraft_renderer PRIVATE symocraft_renderer_vulkan)
    target_compile_definitions(symocraft_renderer PRIVATE SYMOCRAFT_HAS_VULKAN_BACKEND=1)
endif()

if(SYMOCRAFT_BUILD_GAME AND SYMOCRAFT_RENDER_OPENGL)
    add_library(symocraft_renderer_legacy_opengl STATIC
        src/legacy_renderer.cpp src/shader.cpp src/shader_program.cpp src/texture.cpp
        src/gpu_timer.cpp src/performance_memory.cpp)
    target_include_directories(symocraft_renderer_legacy_opengl PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/legacy/include" PRIVATE src
        "${CMAKE_CURRENT_SOURCE_DIR}/../platform/src/graphics_bridge")
    target_link_libraries(symocraft_renderer_legacy_opengl PUBLIC
        symocraft_foundation symocraft_scene PRIVATE symocraft_assets symocraft_platform
        symocraft_glad symocraft_stb_headers symocraft_robin_hood)
    target_compile_features(symocraft_renderer_legacy_opengl PUBLIC cxx_std_20)
    target_compile_options(symocraft_renderer_legacy_opengl PRIVATE /W4 /utf-8)
endif()
