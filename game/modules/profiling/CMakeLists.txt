add_library(symocraft_profiling INTERFACE)
target_include_directories(symocraft_profiling INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(symocraft_profiling INTERFACE cxx_std_20)

if(SYMOCRAFT_ENABLE_TRACY)
    set(tracy_root "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/tracy")
    foreach(required IN ITEMS public/TracyClient.cpp public/tracy/Tracy.hpp LICENSE)
        if(NOT EXISTS "${tracy_root}/${required}")
            message(FATAL_ERROR "SYMOCRAFT_ENABLE_TRACY requires fixed local Tracy v0.14.1: missing ${tracy_root}/${required}")
        endif()
    endforeach()
    file(SHA256 "${tracy_root}/LICENSE" tracy_license_hash)
    if(NOT tracy_license_hash STREQUAL "27ab6b51e57f7e263a6e953d9c8984cd26e69d2f7606bde81e4b3048acf30732")
        message(FATAL_ERROR "Fixed Tracy v0.14.1 original license identity changed")
    endif()
    add_library(symocraft_tracy_client STATIC "${tracy_root}/public/TracyClient.cpp")
    target_compile_features(symocraft_tracy_client PUBLIC cxx_std_20)
    target_include_directories(symocraft_tracy_client SYSTEM PUBLIC "${tracy_root}/public")
    target_compile_definitions(symocraft_tracy_client PUBLIC
        TRACY_ENABLE TRACY_ON_DEMAND TRACY_ONLY_LOCALHOST
        TRACY_NO_BROADCAST TRACY_NO_SYSTEM_TRACING TRACY_NO_SAMPLING
        TRACY_NO_CALLSTACK TRACY_NO_CRASH_HANDLER TRACY_NO_FRAME_IMAGE
        TRACY_NO_CONTEXT_SWITCH TRACY_NO_VSYNC_CAPTURE)
    # Upstream stays unchanged: TRACY_NO_EXIT (including its environment override)
    # is unsupported, and must not be defined here.
    target_link_libraries(symocraft_profiling INTERFACE symocraft_tracy_client)
    target_compile_definitions(symocraft_profiling INTERFACE SYMOCRAFT_TRACY_ENABLED=1)
    install(FILES "${tracy_root}/LICENSE" DESTINATION licenses RENAME Tracy-LICENSE.txt)
endif()
