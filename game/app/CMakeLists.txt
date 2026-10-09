add_library(symocraft_startup STATIC src/startup_options.cpp)
target_compile_features(symocraft_startup PUBLIC cxx_std_20)
target_compile_options(symocraft_startup PRIVATE /W4 /utf-8)

if(NOT SYMOCRAFT_BUILD_GAME)
    return()
endif()

add_executable(SymoCraft src/main.cpp src/application.cpp)
target_link_libraries(SymoCraft PRIVATE symocraft_startup symocraft_foundation symocraft_scene
    symocraft_assets symocraft_ecs symocraft_world symocraft_simulation symocraft_platform
    symocraft_renderer symocraft_telemetry)
target_compile_features(SymoCraft PRIVATE cxx_std_20)
target_compile_options(SymoCraft PRIVATE /W4 /utf-8)
target_sources(SymoCraft PRIVATE "${SYMOCRAFT_REPOSITORY_ROOT}/cmake/windows-utf8.manifest")
symocraft_stage_assets(SymoCraft)
install(TARGETS SymoCraft RUNTIME DESTINATION .)
install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/sdl3/LICENSE.txt" DESTINATION licenses RENAME SDL3-LICENSE.txt)
install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/vendor/stb/LICENSE" DESTINATION licenses RENAME stb-LICENSE.txt)
foreach(asset IN LISTS SYMOCRAFT_REQUIRED_ASSETS)
    get_filename_component(directory "${asset}" DIRECTORY)
    install(FILES "${SYMOCRAFT_REPOSITORY_ROOT}/assets/${asset}" DESTINATION "assets/${directory}")
endforeach()
